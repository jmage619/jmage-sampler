#ifndef JM_SAMPLER_H
#define JM_SAMPLER_H

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
  jack_nframes_t release;
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

//extern jm_key_zone* jm_zones;
//extern int jm_num_zones;

// opaque type for the sampler object
typedef struct JMSampler JMSampler;

#ifdef __cplusplus
extern "C" {
#endif
  void jm_init_key_zone(jm_key_zone* zone);
  int jm_zone_contains(jm_key_zone const * zone, int pitch);

  JMSampler* jm_new_sampler();
  void jm_destroy_sampler(JMSampler* jms);

  void jm_add_zone(JMSampler* jms, jm_key_zone const * zone);
  void jm_get_zone(JMSampler* jms, int index, jm_key_zone* zone);
  void jm_update_zone(JMSampler* jms, int index, jm_key_zone const * zone);
  void jm_remove_zone(JMSampler* jms, int index);
  size_t jm_num_zones(JMSampler* jms);

  void jm_send_msg(JMSampler* jms, jm_msg const * msg);
  int jm_receive_msg(JMSampler* jms, jm_msg* msg);
#ifdef __cplusplus
}
#endif

#endif
