#ifndef ZONE_H
#define ZONE_H

#define MAX_NAME 32
#define MAX_PATH 256
#define NOTE_MIN 0
#define NOTE_MAX 127
#define VEL_MIN 0
#define VEL_MAX 127
#define ORIGIN_DEFAULT 36

namespace jm {
  enum loop_mode {
    LOOP_UNSET = -1,
    LOOP_OFF,
    LOOP_CONTINUOUS,
    LOOP_ONE_SHOT
  };

  struct zone {
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
    char name[MAX_NAME];
    char path[MAX_PATH];
  };

  inline void init_zone(zone& zone) {
    zone.start = 0;
    zone.left = 0;
    zone.low_key = NOTE_MIN;
    zone.high_key = NOTE_MAX;
    zone.origin = ORIGIN_DEFAULT;
    zone.low_vel = VEL_MIN;
    zone.high_vel = VEL_MAX;
    zone.amp = 1.0;
    zone.attack = 0;
    zone.hold = 0;
    zone.decay = 0;
    zone.sustain = 1.0;
    zone.release = 0;
    zone.long_tail = 0;
    zone.pitch_corr = 0.0;
    zone.mode = LOOP_OFF;
    zone.group = 0;
    zone.off_group = 0;
    zone.crossfade = 0;
  }

  inline bool zone_contains(const zone& zone, int pitch, int velocity) {
    return pitch >= zone.low_key && pitch <= zone.high_key &&
      velocity >= zone.low_vel && velocity <= zone.high_vel;
  }
}

#endif