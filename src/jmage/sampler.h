#ifndef JMAGE_SAMPLER_H
#define JMAGE_SAMPLER_H

#include <jack/types.h>

#define VOL_STEPS 17

typedef jack_default_audio_sample_t sample_t;

typedef struct {
  sample_t* wave[2];
  jack_nframes_t wave_length;
  jack_nframes_t start;
  jack_nframes_t left;
  jack_nframes_t right;
  int lower_bound;
  int upper_bound;
  int origin;
  double amp;
  double rel_time;
  double pitch_corr;
  int loop_on;
  jack_nframes_t crossfade;
} jm_key_zone;

typedef enum {MT_VOLUME} msg_type;

typedef struct {
  msg_type type;
  union {
    int i;
  } data;
} jm_msg;

extern jm_key_zone* jm_zones;
extern int jm_num_zones;

#ifdef __cplusplus
extern "C" {
#endif
  void jm_init_key_zone(jm_key_zone* zone);
  int jm_zone_contains(jm_key_zone* zone, int pitch);

  jack_client_t* jm_init_sampler();
  void jm_destroy_sampler(jack_client_t* client);

  jm_msg* jm_new_msg();
  void jm_destroy_msg(jm_msg* msg);
  void jm_send_msg(jm_msg* msg);
  int jm_receive_msg(jm_msg** msg);
#ifdef __cplusplus
}
#endif

#endif
