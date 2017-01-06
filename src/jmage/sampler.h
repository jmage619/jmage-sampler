#ifndef JM_SAMPLER_H
#define JM_SAMPLER_H

#include <linux/limits.h>
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

typedef enum {
  LOOP_UNSET = -1,
  LOOP_OFF,
  LOOP_CONTINUOUS,
  LOOP_ONE_SHOT
} loop_mode;

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
  int long_tail;
  double pitch_corr;
  loop_mode mode;
  int group;
  int off_group;
  int crossfade;
  // some meta info only used by ui
  char name[PATH_MAX];
  char path[PATH_MAX];
} jm_key_zone;

typedef enum {
MT_VOLUME,
MT_CHANNEL
} msg_type;

typedef struct {
  msg_type type;
  union {
    int i;
  } data;
} jm_msg;

//extern jm_key_zone* jm_zones;
//extern int jm_num_zones;

// opaque type for the zone list
//typedef struct JMZoneList JMZoneList;

// opaque type for the sampler object
typedef struct JMSampler JMSampler;

#ifdef __cplusplus
extern "C" {
#endif
  int jm_parse_wave(jm_wave* wav, char const * path);
  void jm_destroy_wave(jm_wave* wav);

  void jm_init_key_zone(jm_key_zone* zone);
  void jm_zone_set_name(jm_key_zone* zone, char const * name);
  void jm_zone_set_path(jm_key_zone* zone, char const * path);
  int jm_zone_contains(jm_key_zone const * zone, int pitch, int velocity);

  // wrappers for JMZoneList
  /*JMZoneList* jm_new_zonelist();
  void jm_destroy_zonelist(JMZoneList* jzl);
  void jm_zonelist_insert(JMZoneList* jzl, int index, jm_key_zone const * zone);
  int jm_zonelist_get(JMZoneList* jzl, int index, jm_key_zone* zone);
  int jm_zonelist_set(JMZoneList* jzl, int index, jm_key_zone const * zone);
  int jm_zonelist_erase(JMZoneList* jzl, int index);
  size_t jm_zonelist_size(JMZoneList* jzl);
  void jm_zonelist_lock(JMZoneList* jzl);
  void jm_zonelist_unlock(JMZoneList* jzl);

  // wrappers for JMSampler
  JMSampler* jm_new_sampler(JMZoneList* zones);
  */
  void jm_destroy_sampler(JMSampler* jms);

  void jm_send_msg(JMSampler* jms, jm_msg const * msg);
  int jm_receive_msg(JMSampler* jms, jm_msg* msg);
#ifdef __cplusplus
}
#endif

#endif
