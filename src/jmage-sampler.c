#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/transport.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <limits.h>
#include "jmage/structures.h"

#define UE_Q_SIZE 10
#define WAV_OFF_Q_SIZE 10

jack_client_t *client;
jack_port_t *input_port;
jack_port_t *output_port1;
jack_port_t *output_port2;
jack_nframes_t wave_length;
sample_t *wave1;
sample_t *wave2;
double amp[17];
volatile int level = 1;
playhead_list playheads;
struct key_zone zones[1];

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

  ph_list_iterator it;
  struct playhead* ph_p;
  jack_nframes_t i;
  for (i = 0; i <= nframes; i++) {
    if (cur_event < event_count) {
      while (i == event.time) {
        if ((event.buffer[0] & 0xf0) == 0x90) {
          struct playhead ph = zone_to_ph(zones, 1, event.buffer[1]);

          if (ph_list_size(&playheads) >= WAV_OFF_Q_SIZE)
            ph_list_remove_last(&playheads);

          ph_list_add(&playheads, ph);
          printf("poly: %zu note: %f\n", ph_list_size(&playheads), 12 * log2(ph.speed));
        }
        cur_event++;
        if (cur_event == event_count)
          break;
        jack_midi_event_get(&event, midi_buf, cur_event);
      }
    }

    it = ph_list_get_iterator(&playheads);
    while ((ph_p = ph_list_iter_next(&it)) != NULL) {
      buffer1[i] += amp[level] * wave1[(jack_nframes_t) ph_p->position];
      buffer2[i] += amp[level] * wave2[(jack_nframes_t) ph_p->position];
      ph_p->position += ph_p->speed;
      if ((jack_nframes_t) ph_p->position >= wave_length) {
        ph_list_iter_remove(&it);
        printf("poly: %zu\n", ph_list_size(&playheads));
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
  // Build the wave tables
  FILE* wav;
  char text[5];
  text[4] = '\0';

  uint32_t num32;
  uint16_t num16;
  //wav = fopen("shawn.wav", "rb");
  //wav = fopen("Glass.wav", "rb");
  wav = fopen("rhodes_note.wav", "rb");
  //wav = fopen("Leaving_rh_remix.wav", "rb");

  fread(text, 1, 4, wav);
  printf("ChunkID: \"%s\"\n", text);
  fread(&num32, 4, 1, wav);
  printf("ChunkSize: %" PRIu32 "\n", num32);
  fread(text, 1, 4, wav);
  printf("Format: \"%s\"\n", text);
  fread(text, 1, 4, wav);
  printf("Subchunk1ID: \"%s\"\n", text);
  fread(&num32, 4, 1, wav);
  printf("Subchunk1Size: %" PRIu32 "\n", num32);
  fread(&num16, 2, 1, wav);
  printf("AudioFormat: %" PRIu16 "\n", num16);
  fread(&num16, 2, 1, wav);
  printf("NumChannels: %" PRIu16 "\n", num16);
  fread(&num32, 4, 1, wav);
  printf("SampleRate: %" PRIu32 "\n", num32);
  fread(&num32, 4, 1, wav);
  printf("ByteRate: %" PRIu32 "\n", num32);
  fread(&num16, 2, 1, wav);
  printf("BlockAlign: %" PRIu16 "\n", num16);
  int block_align = num16;
  fread(&num16, 2, 1, wav);
  printf("BitsPerSample: %" PRIu16 "\n", num16);
  fread(text, 1, 4, wav);
  printf("Subchunk2ID: \"%s\"\n", text);
  fread(&num32, 4, 1, wav);
  printf("Subchunk2Size: %" PRIu32 "\n", num32);
  int data_size = num32;

  wave_length = data_size / block_align;
  printf("calculated wave length: %i\n", wave_length);
  wave1 = (sample_t*) malloc(sizeof(sample_t) * wave_length);
  wave2 = (sample_t*) malloc(sizeof(sample_t) * wave_length);

  // assuming 16 bit and 2 channel
  int16_t sample;
  int i;
  for (i = 0; i < wave_length; i++) {
    fread(&sample, 2, 1, wav);
    wave1[i] = sample;
    fread(&sample, 2, 1, wav);
    wave2[i] = sample;
  }

  fclose(wav);

  for (i = 0; i < 17; i++) {
    amp[i]  = pow(10, - (16 - i) * 6 / 20.);
  }

  amp[0] = 0.0;
  
  init_ph_list(&playheads, WAV_OFF_Q_SIZE);

  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    return 1;
  }

  zones[0].origin = 0x30;
  zones[0].lower_bound = INT_MIN;
  zones[0].upper_bound = INT_MAX;
  zones[0].wave[0] = wave1;
  zones[0].wave[1] = wave2;

  int c;
  while (1) {
    c=getch();
    //printf("key typed: %c\n",c);

    switch(c) {
      case 'x':
        jack_deactivate(client);
        jack_client_close(client);

        free(wave1);
        free(wave2);
        destroy_ph_list(&playheads);
        return 0;
      case '[':
        if (level > 0)
          level--;
        printf("level: %i\n", level);
        continue;
      case ']':
        if (level < 16)
          level++;
        printf("level: %i\n", level);
        continue;
      default:
        continue;
    }
  }
}
