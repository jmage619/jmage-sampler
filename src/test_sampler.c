#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <jack/types.h>
#include <sndfile.h>
#include <termios.h>
#include "jmage/sampler.h"

#define NUM_ZONES 1
#define RELEASE_TIME  (44100. / 1000.)

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

int main() {
  jm_num_zones = NUM_ZONES;
  jm_zones = (jm_key_zone*) malloc(sizeof(jm_key_zone) * jm_num_zones);
  int i;
  for (i = 0; i < NUM_ZONES; i++)
    jm_init_key_zone(&jm_zones[i]);

  //wav = fopen("shawn.wav", "rb");
  //wav = fopen("Glass.wav", "rb");
  //wav = fopen("Leaving_rh_remix.wav", "rb");

  /**** load wav 1 ***/
  jm_zones[0].origin = 48;
  jm_zones[0].lower_bound = INT_MIN;
  jm_zones[0].upper_bound = INT_MAX;
  jm_zones[0].rel_time = RELEASE_TIME;

  SF_INFO sf_info;
  sf_info.format = 0;
  
  //SNDFILE* wav = sf_open("rhodes_note.wav", SFM_READ, &sf_info);
  SNDFILE* wav = sf_open("afx.wav", SFM_READ, &sf_info);

  printf("wave length: %" PRIi64 "\n", sf_info.frames);
  jm_zones[0].start = 0;
  jm_zones[0].left = 44100;
  jm_zones[0].right = 3 * 44100 + 5 * 44100 / 8;
  jm_zones[0].wave[0] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  jm_zones[0].wave[1] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  jm_zones[0].wave_length = sf_info.frames;
  jm_zones[0].amp = 1.0;
  jm_zones[0].pitch_corr = 0.0;
  jm_zones[0].loop_on = 1;
  jm_zones[0].crossfade = 22050;

  // assuming 2 channel
  double frame[2];
  sf_count_t f;
  for (f = 0; f < sf_info.frames; f++) {
    sf_readf_double(wav, frame, 1);
    jm_zones[0].wave[0][f] = frame[0];
    jm_zones[0].wave[1][f] = frame[1];
  }

  sf_close(wav);

  /**** load wav 2 ***/
  /*jm_zones[1].origin = 50;
  jm_zones[1].lower_bound = INT_MIN;
  jm_zones[1].upper_bound = INT_MAX;
  jm_zones[1].amp = 0.1;
  jm_zones[1].pitch_corr = 0.5;
  jm_zones[1].rel_time = RELEASE_TIME;

  sf_info.format = 0;
  
  wav = sf_open("Glass.wav", SFM_READ, &sf_info);

  printf("wave length: %" PRIi64 "\n", sf_info.frames);
  jm_zones[1].right =  sf_info.frames;
  jm_zones[1].wave[0] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  jm_zones[1].wave[1] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  jm_zones[1].wave_length = sf_info.frames;

  // assuming 2 channel
  for (i = 0; i < sf_info.frames; i++) {
    sf_readf_double(wav, frame, 1);
    jm_zones[1].wave[0][i] = frame[0];
    jm_zones[1].wave[1][i] = frame[1];
  }

  sf_close(wav);
  */
  /* end load wav 2 ***/

  jack_client_t* client;
  if ((client = jm_init_sampler()) == NULL) {
    fprintf (stderr, "cannot activate jmage");
    return 1;
  }

  int c;
  while (1) {
    c = getch();
    //printf("key typed: %c\n",c);

    switch(c) {
      case 'x':
        jm_destroy_sampler(client);

        int i;
        for (i = 0; i < NUM_ZONES; i ++) {
          free(jm_zones[i].wave[0]);
          free(jm_zones[i].wave[1]);
        }
        free(jm_zones);
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
