#ifndef JM_SAMPLER_H
#define JM_SAMPLER_H

#include <jack/types.h>

#define VOL_STEPS 17
#define NOTE_MIN 0
#define NOTE_MAX 127
#define VEL_MIN 0
#define VEL_MAX 127
#define ORIGIN_DEFAULT 36

typedef jack_default_audio_sample_t sample_t;

typedef struct {
  float* wave;
  int num_channels;
  int length;
  int left;
  int right;
  int has_loop;
} jm_wave;

typedef struct {
  float* wave;
  int num_channels;
  int wave_length;
  int start;
  int left;
  int right;
  int low_key;
  int high_key;
  int origin;
  int low_vel;
  int high_vel;
  float amp;
  int attack;
  int hold;
  int decay;
  float sustain;
  int release;
  double pitch_corr;
  int loop_on;
  int crossfade;
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
  int jm_parse_wave(jm_wave* wav, char const * path);
  void jm_destroy_wave(jm_wave* wav);

  void jm_init_key_zone(jm_key_zone* zone);
  int jm_zone_contains(jm_key_zone const * zone, int pitch, int velocity);

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
