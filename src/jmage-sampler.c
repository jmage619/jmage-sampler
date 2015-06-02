#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>
#include <sndfile.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <limits.h>
#include "jmage/structures.h"

#define UE_Q_SIZE 10
#define WAV_OFF_Q_SIZE 10
#define VOL_STEPS 17
#define NUM_ZONES 1
#define RELEASE_TIME  (44100. / 1000.)

jack_client_t *client;
jack_port_t *input_port;
jack_port_t *output_port1;
jack_port_t *output_port2;
double amp[VOL_STEPS];
volatile int level = VOL_STEPS - 1;
playhead_list playheads;
int sustain_on = 0;
struct key_zone zones[NUM_ZONES];

void
usage ()

{
  fprintf (stderr, "\n"
"usage: ghetto_sampler is fucking great! \n"
"              [ --fake OR -f does nothing ]\n"
);
}

// crazy shit to imitate getch in linux / os x
char getch() {
  struct termios old, new;
  tcgetattr(0, &old);
  new = old;
  new.c_lflag &= ~ (ICANON | ECHO);
  tcsetattr(0, TCSANOW, &new);
  char c = getchar();
  tcsetattr(0, TCSANOW, &old);
  return c;
}

void
process_audio (jack_nframes_t nframes) 
{

  sample_t *buffer1 = (sample_t *) jack_port_get_buffer (output_port1, nframes);
  sample_t *buffer2 = (sample_t *) jack_port_get_buffer (output_port2, nframes);

  memset (buffer1, 0, sizeof (jack_default_audio_sample_t) * nframes);
  memset (buffer2, 0, sizeof (jack_default_audio_sample_t) * nframes);

  void* midi_buf = jack_port_get_buffer(input_port, nframes);

  uint32_t event_count = jack_midi_get_event_count(midi_buf);
  uint32_t cur_event = 0;
  jack_midi_event_t event;
  if (event_count > 0)
    jack_midi_event_get(&event, midi_buf, cur_event);

  ph_list_el* pel;
  jack_nframes_t n;
  for (n = 0; n <= nframes; n++) {
    if (cur_event < event_count) {
      while (n == event.time) {
        if ((event.buffer[0] & 0xf0) == 0x90) {
          if (sustain_on) {
            for (pel = ph_list_head(&playheads); pel != NULL; pel = pel->next) {
              if (pel->ph.pitch == event.buffer[1])
                pel->ph.released = 1;
            }
          }
          int i;
          for (i = 0; i < NUM_ZONES; i++) {
            if (in_zone(&zones[i], event.buffer[1])) {
              struct playhead ph;
              zone_to_ph(&zones[i], &ph, event.buffer[1], event.buffer[2]);

              if (ph_list_size(&playheads) >= WAV_OFF_Q_SIZE)
                ph_list_remove_last(&playheads);

              ph_list_add(&playheads, &ph);
              printf("event: note on;  note: %i; vel: %i; amp: %f\n", event.buffer[1], event.buffer[2], ph.amp);
            }
          }
        }
        else if ((event.buffer[0] & 0xf0) == 0x80) {
          printf("event: note off; note: %i\n", event.buffer[1]);
          for (pel = ph_list_head(&playheads); pel != NULL; pel = pel->next) {
            if (pel->ph.pitch == event.buffer[1]) {
              if (sustain_on) {
                pel->ph.note_off = 1;
              }
              else
                pel->ph.released = 1;
            }
          }
        }
        else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
          if (event.buffer[2] >= 64) {
            sustain_on = 1;
            printf("sustain on\n");
          }
          else {
            for (pel = ph_list_head(&playheads); pel != NULL; pel = pel->next) {
              if (pel->ph.note_off)
                pel->ph.released = 1;
            }

            sustain_on = 0;
            printf("sustain off\n");
          }
        }
        else if (event.buffer[0] != 0xfe)
          printf("event: 0x%x\n", event.buffer[0]);

        cur_event++;
        if (cur_event == event_count)
          break;
        jack_midi_event_get(&event, midi_buf, cur_event);
      }
    }

    for (pel = ph_list_head(&playheads); pel != NULL; pel = pel->next) {
      if (pel->ph.released) {
        double rel_amp = - pel->ph.rel_time / RELEASE_TIME + 1.0;
        pel->ph.rel_time++;
        buffer1[n] += amp[level] * rel_amp * pel->ph.amp * pel->ph.wave[0][(jack_nframes_t) pel->ph.position];
        buffer2[n] += amp[level] * rel_amp * pel->ph.amp * pel->ph.wave[1][(jack_nframes_t) pel->ph.position];
      }
      else {
        buffer1[n] += amp[level] * pel->ph.amp * pel->ph.wave[0][(jack_nframes_t) pel->ph.position];
        buffer2[n] += amp[level] * pel->ph.amp * pel->ph.wave[1][(jack_nframes_t) pel->ph.position];
      }
      pel->ph.position += pel->ph.speed;
      if ((jack_nframes_t) pel->ph.position >= pel->ph.wave_length || pel->ph.rel_time >= RELEASE_TIME) {
        ph_list_remove(&playheads, pel);
      }
    }
  }
}

int
process (jack_nframes_t nframes, void *arg)
{
  process_audio (nframes);
  return 0;
}

int
main (int argc, char *argv[])
{
  int option_index;
  int opt;
  char client_name[] = "ghetto_sampler";

  const char *options = "h";
  struct option long_options[] =
  {
    {"help", 0, 0, 'h'},
    {0, 0, 0, 0}
  };
  
  while ((opt = getopt_long (argc, argv, options, long_options, &option_index)) != EOF) {
    switch (opt) {
    case 'h':
      usage ();
      return -1;
    }
  }

  jack_status_t status; 
  if ((client = jack_client_open (client_name,JackNullOption,&status)) == 0) {
    fprintf (stderr, "jack server not running?\n");
    return 1;
  }
  jack_set_process_callback (client, process, 0);
  input_port = jack_port_register (client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  output_port1 = jack_port_register (client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_port2 = jack_port_register (client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  //wav = fopen("shawn.wav", "rb");
  //wav = fopen("Glass.wav", "rb");
  //wav = fopen("Leaving_rh_remix.wav", "rb");

  zones[0].origin = 0x30;
  zones[0].lower_bound = INT_MIN;
  zones[0].upper_bound = INT_MAX;

  SF_INFO sf_info;
  sf_info.format = 0;
  
  SNDFILE* wav = sf_open("rhodes_note.wav", SFM_READ, &sf_info);

  printf("wave length: %" PRIi64 "\n", sf_info.frames);
  zones[0].wave_length =  sf_info.frames;
  zones[0].wave[0] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  zones[0].wave[1] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);

  // assuming 2 channel
  double frame[2];
  sf_count_t i;
  for (i = 0; i < sf_info.frames; i++) {
    sf_readf_double(wav, frame, 1);
    zones[0].wave[0][i] = frame[0];
    zones[0].wave[1][i] = frame[1];
  }

  sf_close(wav);

  for (i = 0; i < VOL_STEPS; i++) {
    //amp[i]  = (double) i / (VOL_STEPS - 1);
    amp[i]  = 1 / 90. * (pow(10, 2 * i / (VOL_STEPS - 1.0)) - 1);
  }
  amp[VOL_STEPS - 1] = 1.0;
  /*for (i = 0; i < VOL_STEPS; i++) {
    printf("step: %i gain: %f\n", i, amp[i]);
  }
  */
  
  init_ph_list(&playheads, WAV_OFF_Q_SIZE);

  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    return 1;
  }

  int c;
  while (1) {
    c=getch();
    //printf("key typed: %c\n",c);

    switch(c) {
      case 'x':
        jack_deactivate(client);
        jack_client_close(client);

        free(zones[0].wave[0]);
        free(zones[0].wave[1]);
        destroy_ph_list(&playheads);
        return 0;
      case '[':
        if (level > 0)
          level--;
        printf("level: %i\n", level);
        continue;
      case ']':
        if (level < VOL_STEPS - 1)
          level++;
        printf("level: %i\n", level);
        continue;
      default:
        continue;
    }
  }
}
