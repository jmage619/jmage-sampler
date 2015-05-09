#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <jack/jack.h>
#include <jack/transport.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>
#include <limits.h>
#include "jmage/structures.h"

#define UE_Q_SIZE 10
#define WAV_OFF_Q_SIZE 1

jack_client_t *client;
jack_port_t *output_port1;
jack_port_t *output_port2;
jack_nframes_t wave_length;
sample_t *wave1;
sample_t *wave2;
double amp[17];
volatile int level = 1;
jm_queue user_events;
playhead_list playheads;

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

  struct playhead ph;
  while (jm_q_remove(&user_events, &ph) != NULL) {
    if (ph_list_size(&playheads) < WAV_OFF_Q_SIZE)
      ph_list_add(&playheads, ph);
      printf("poly: %zu note: %f\n", ph_list_size(&playheads), 12 * log2(ph.speed));
  }

  memset (buffer1, 0, sizeof (jack_default_audio_sample_t) * nframes);
  memset (buffer2, 0, sizeof (jack_default_audio_sample_t) * nframes);

  ph_list_iterator it = ph_list_get_iterator(&playheads);
  struct playhead* ph_p;
  while ((ph_p = ph_list_iter_next(&it)) != NULL) {
    jack_nframes_t to_copy = nframes;
    if (wave_length - ph_p->position < nframes * ph_p->speed)
      to_copy = (jack_nframes_t) ((wave_length - ph_p->position) / ph_p->speed);

    int i;
    for (i = 0; i < to_copy; i++) {
      buffer1[i] += amp[level] * wave1[(jack_nframes_t) (ph_p->position + i * ph_p->speed)];
      buffer2[i] += amp[level] * wave2[(jack_nframes_t) (ph_p->position + i * ph_p->speed)];
    }
    ph_p->position += to_copy * ph_p->speed;
    if ((jack_nframes_t) ((wave_length - ph_p->position) / ph_p->speed) == 0) {
      ph_list_iter_remove(&it);
      printf("poly: %zu\n", ph_list_size(&playheads));
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
  output_port1 = jack_port_register (client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_port2 = jack_port_register (client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  // Build the wave tables
  FILE* wav;
  char text[5];
  text[4] = '\0';

  uint32_t num32;
  uint16_t num16;
  //wav = fopen("shawn.wav", "rb");
  wav = fopen("Glass.wav", "rb");
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
  
  jm_init_queue(&user_events, sizeof(struct playhead), UE_Q_SIZE);
  init_ph_list(&playheads, WAV_OFF_Q_SIZE);

  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    return 1;
  }

  struct key_zone zones[1];
  zones[0].origin = 12;
  zones[0].lower_bound = INT_MIN;
  zones[0].upper_bound = INT_MAX;
  zones[0].wave[0] = wave1;
  zones[0].wave[1] = wave2;

  int octave = 0;
  int pitch;

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
        jm_destroy_queue(&user_events);
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
      case '1':
        octave -= 1;
        printf("octave: %i\n", octave);
        continue;
      case '2':
        octave += 1;
        printf("octave: %i\n", octave);
        continue;
      case 'a':
        pitch = 0 + 12 * octave;
        break;
      case 'w':
        pitch = 1 + 12 * octave;
        break;
      case 's':
        pitch = 2 + 12 * octave;
        break;
      case 'e':
        pitch = 3 + 12 * octave;
        break;
      case 'd':
        pitch = 4 + 12 * octave;
        break;
      case 'f':
        pitch = 5 + 12 * octave;
        break;
      case 't':
        pitch = 6 + 12 * octave;
        break;
      case 'g':
        pitch = 7 + 12 * octave;
        break;
      case 'y':
        pitch = 8 + 12 * octave;
        break;
      case 'h':
        pitch = 9 + 12 * octave;
        break;
      case 'u':
        pitch = 10 + 12 * octave;
        break;
      case 'j':
        pitch = 11 + 12 * octave;
        break;
      case 'k':
        pitch = 12 + 12 * octave;
        break;
      case 'o':
        pitch = 13 + 12 * octave;
        break;
      case 'l':
        pitch = 14 + 12 * octave;
        break;
      case 'p':
        pitch = 15 + 12 * octave;
        break;
      case ';':
        pitch = 16 + 12 * octave;
        break;
      case '\'':
        pitch = 17 + 12 * octave;
        break;
      default:
        continue;
    }
    //printf("pitch: %i\n", pitch);
    struct playhead ph = zone_to_ph(zones, 1, pitch);
    jm_q_add(&user_events, &ph);
  }
}
