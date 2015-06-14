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
#include "jmage/components.h"

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
volatile int level = VOL_STEPS - 8;
PlayheadList playheads(WAV_OFF_Q_SIZE);
int sustain_on = 0;
KeyZone zones[NUM_ZONES];

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
  struct termios term_old, term_new;
  tcgetattr(0, &term_old);
  term_new = term_old;
  term_new.c_lflag &= ~ (ICANON | ECHO);
  tcsetattr(0, TCSANOW, &term_new);
  char c = getchar();
  tcsetattr(0, TCSANOW, &term_old);
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
            for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
              if (pel->ph.pitch == event.buffer[1])
                pel->ph.state = Playhead::RELEASED;
            }
          }
          int i;
          for (i = 0; i < NUM_ZONES; i++) {
            if (zones[i].contains(event.buffer[1])) {
              Playhead ph;
              zones[i].to_ph(ph, event.buffer[1], event.buffer[2]);

              if (playheads.size() >= WAV_OFF_Q_SIZE)
                playheads.remove_last();

              playheads.add(ph);
              printf("event: note on;  note: %i; vel: %i; amp: %f\n", event.buffer[1], event.buffer[2], ph.amp);
            }
          }
        }
        else if ((event.buffer[0] & 0xf0) == 0x80) {
          printf("event: note off; note: %i\n", event.buffer[1]);
          for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
            if (pel->ph.pitch == event.buffer[1]) {
              if (sustain_on) {
                pel->ph.state = Playhead::NOTE_OFF;
              }
              else
                pel->ph.state = Playhead::RELEASED;
            }
          }
        }
        else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
          if (event.buffer[2] >= 64) {
            sustain_on = 1;
            printf("sustain on\n");
          }
          else {
            for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
              if (pel->ph.state == Playhead::NOTE_OFF)
                pel->ph.state = Playhead::RELEASED;
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

    for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
      double values[2];
      pel->ph.get_values(values);
      buffer1[n] += amp[level] * values[0];
      buffer2[n] += amp[level] * values[1];

      pel->ph.inc();
      if (pel->ph.state == Playhead::FINISHED)
        playheads.remove(pel);
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

  /**** load wav 1 ***/
  zones[0].origin = 48;
  zones[0].lower_bound = INT_MIN;
  zones[0].upper_bound = INT_MAX;
  zones[0].rel_time = RELEASE_TIME;

  SF_INFO sf_info;
  sf_info.format = 0;
  
  //SNDFILE* wav = sf_open("rhodes_note.wav", SFM_READ, &sf_info);
  SNDFILE* wav = sf_open("afx.wav", SFM_READ, &sf_info);

  printf("wave length: %" PRIi64 "\n", sf_info.frames);
  zones[0].start = 0;
  zones[0].left = 44100;
  zones[0].right = 3 * 44100 + 5 * 44100 / 8;
  zones[0].wave[0] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  zones[0].wave[1] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  zones[0].wave_length = sf_info.frames;
  zones[0].amp = 1.0;
  zones[0].pitch_corr = 0.0;
  zones[0].loop_on = true;
  zones[0].crossfade = 22050;

  // assuming 2 channel
  double frame[2];
  sf_count_t i;
  for (i = 0; i < sf_info.frames; i++) {
    sf_readf_double(wav, frame, 1);
    zones[0].wave[0][i] = frame[0];
    zones[0].wave[1][i] = frame[1];
  }

  sf_close(wav);

  /**** load wav 2 ***/
  /*zones[1].origin = 50;
  zones[1].lower_bound = INT_MIN;
  zones[1].upper_bound = INT_MAX;
  zones[1].amp = 0.1;
  zones[1].pitch_corr = 0.5;
  zones[1].rel_time = RELEASE_TIME;

  sf_info.format = 0;
  
  wav = sf_open("Glass.wav", SFM_READ, &sf_info);

  printf("wave length: %" PRIi64 "\n", sf_info.frames);
  zones[1].right =  sf_info.frames;
  zones[1].wave[0] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  zones[1].wave[1] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  zones[1].wave_length = sf_info.frames;

  // assuming 2 channel
  for (i = 0; i < sf_info.frames; i++) {
    sf_readf_double(wav, frame, 1);
    zones[1].wave[0][i] = frame[0];
    zones[1].wave[1][i] = frame[1];
  }

  sf_close(wav);
  */
  /* end load wav 2 ***/

  for (i = 0; i < VOL_STEPS; i++) {
    //amp[i]  = (double) i / (VOL_STEPS - 1);
    amp[i]  = 1 / 100. * pow(10, 2 * i / (VOL_STEPS - 1.0));
  }
  amp[0] = 0.0;
  /*for (i = 0; i < VOL_STEPS; i++) {
    printf("step: %i gain: %f\n", i, amp[i]);
  }
  */
  
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

        int i;
        for (i = 0; i < NUM_ZONES; i ++) {
          free(zones[i].wave[0]);
          free(zones[i].wave[1]);
        }
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
