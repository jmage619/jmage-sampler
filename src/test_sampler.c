#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <jack/types.h>
#include <sndfile.h>
#include <termios.h>
#include "jmage/sampler.h"

#define NUM_ZONES 1
#define RELEASE_TIME  (44100 / 1000)
//#define RELEASE_TIME  (44100)

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
  //jm_num_zones = NUM_ZONES;
  //jm_zones = (jm_key_zone*) malloc(sizeof(jm_key_zone) * jm_num_zones);
  //int i;
  jm_key_zone zone1;
  jm_init_key_zone(&zone1);

  //wav = fopen("shawn.wav", "rb");
  //wav = fopen("Glass.wav", "rb");
  //wav = fopen("Leaving_rh_remix.wav", "rb");

  /**** load wav 1 ***/
  zone1.origin = 48;
  zone1.lower_bound = INT_MIN;
  zone1.upper_bound = INT_MAX;
  zone1.release = RELEASE_TIME;

  SF_INFO sf_info;
  sf_info.format = 0;
  
  printf("opening wav\n");
  //SNDFILE* wav = sf_open("rhodes_note.wav", SFM_READ, &sf_info);
  SNDFILE* wav = sf_open("afx.wav", SFM_READ, &sf_info);

  printf("wave length: %" PRIi64 "\n", sf_info.frames);
  zone1.start = 0;
  zone1.left = 44100;
  zone1.right = 3 * 44100 + 5 * 44100 / 8;
  sample_t* wave[2];
  wave[0] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);
  wave[1] = (sample_t*) malloc(sizeof(sample_t) * sf_info.frames);

  zone1.wave[0] = wave[0];
  zone1.wave[1] = wave[1];
  zone1.wave_length = sf_info.frames;
  zone1.amp = 1.0;
  zone1.pitch_corr = 0.0;
  zone1.loop_on = 1;
  zone1.crossfade = 22050;

  // assuming 2 channel
  double frame[2];
  sf_count_t f;
  for (f = 0; f < sf_info.frames; f++) {
    sf_readf_double(wav, frame, 1);
    zone1.wave[0][f] = frame[0];
    zone1.wave[1][f] = frame[1];
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

  JMSampler* jms;
  if ((jms = jm_new_sampler()) == NULL) {
    fprintf (stderr, "cannot activate jmage\n");
    return 1;
  }

  int c;
  int level = VOL_STEPS - 8;

  jm_msg msg;
  msg.type = MT_VOLUME;
  msg.data.i = level;
  jm_send_msg(jms, &msg);

  printf("adding zone\n");
  jm_add_zone(jms, &zone1);

  while (1) {
    c = getch();
    //printf("key typed: %c\n",c);

    switch(c) {
      case 'x':
        jm_remove_zone(jms, 0);
        jm_destroy_sampler(jms);
        free(wave[0]);
        free(wave[1]);
        //int i;
        //for (i = 0; i < NUM_ZONES; i ++) {
        //}
        return 0;
      case '[':
        if (level > 0) {
          level--;
          msg.type = MT_VOLUME;
          msg.data.i = level;
          jm_send_msg(jms, &msg);
        }
        printf("level: %i\n", level);
        continue;
      case ']':
        if (level < VOL_STEPS - 1) {
          level++;
          msg.type = MT_VOLUME;
          msg.data.i = level;
          jm_send_msg(jms, &msg);
        }
        printf("level: %i\n", level);
        continue;
      default:
        continue;
    }
  }
}
