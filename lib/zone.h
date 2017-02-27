#ifndef ZONE_H
#define ZONE_H

#define MAX_NAME 32
#define MAX_PATH 256
#define NOTE_MIN 0
#define NOTE_MAX 127
#define VEL_MIN 0
#define VEL_MAX 127
#define ORIGIN_DEFAULT 36

typedef enum {
  ZONE_NAME,
  ZONE_AMP,
  ZONE_ORIGIN,
  ZONE_LOW_KEY,
  ZONE_HIGH_KEY,
  ZONE_LOW_VEL,
  ZONE_HIGH_VEL,
  ZONE_PITCH,
  ZONE_START,
  ZONE_LEFT,
  ZONE_RIGHT,
  ZONE_LOOP_MODE,
  ZONE_CROSSFADE,
  ZONE_GROUP,
  ZONE_OFF_GROUP,
  ZONE_ATTACK,
  ZONE_HOLD,
  ZONE_DECAY,
  ZONE_SUSTAIN,
  ZONE_RELEASE,
  ZONE_LONG_TAIL, // not used but needed as placeholder to not screw up PATH index
  ZONE_PATH
} jm_zone_params;

typedef enum {
  LOOP_UNSET = -1,
  LOOP_OFF,
  LOOP_CONTINUOUS,
  LOOP_ONE_SHOT
} jm_loop_mode;

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
  jm_loop_mode loop_mode;
  int group;
  int off_group;
  int crossfade;
  // some meta info only used by ui
  char name[MAX_NAME];
  char path[MAX_PATH];
} jm_zone;

inline void jm_init_zone(jm_zone* zone) {
  zone->start = 0;
  zone->left = 0;
  zone->low_key = NOTE_MIN;
  zone->high_key = NOTE_MAX;
  zone->origin = ORIGIN_DEFAULT;
  zone->low_vel = VEL_MIN;
  zone->high_vel = VEL_MAX;
  zone->amp = 1.0;
  zone->attack = 0;
  zone->hold = 0;
  zone->decay = 0;
  zone->sustain = 1.0;
  zone->release = 0;
  zone->long_tail = 0;
  zone->pitch_corr = 0.0;
  zone->loop_mode = LOOP_OFF;
  zone->group = 0;
  zone->off_group = 0;
  zone->crossfade = 0;
}

inline int jm_zone_contains(const jm_zone* zone, int pitch, int velocity) {
  return pitch >= zone->low_key && pitch <= zone->high_key &&
    velocity >= zone->low_vel && velocity <= zone->high_vel;
}

#endif
