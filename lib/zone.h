#ifndef ZONE_H
#define ZONE_H

#include <vector>
#include <cstring>
#include <cstdio>

#define MAX_NAME 32
#define MAX_PATH 256
#define NOTE_MIN 0
#define NOTE_MAX 127
#define VEL_MIN 0
#define VEL_MAX 127
#define ORIGIN_DEFAULT 36

namespace jm {
  enum zone_params {
    ZONE_NAME,
    ZONE_AMP,
    ZONE_MUTE,
    ZONE_SOLO,
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
  };

  enum loop_mode {
    LOOP_UNSET = -1,
    LOOP_OFF,
    LOOP_CONTINUOUS,
    LOOP_ONE_SHOT
  };

  struct zone {
    float* wave;
    int num_channels;
    int sample_rate;
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
    int mute;
    int solo;
    int attack;
    int hold;
    int decay;
    float sustain;
    int release;
    int long_tail;
    double pitch_corr;
    jm::loop_mode loop_mode;
    int group;
    int off_group;
    int crossfade;
    // some meta info only used by ui
    char name[MAX_NAME];
    char path[MAX_PATH];
  };

  inline void init_zone(jm::zone* zone) {
    zone->start = 0;
    zone->left = 0;
    zone->low_key = NOTE_MIN;
    zone->high_key = NOTE_MAX;
    zone->origin = ORIGIN_DEFAULT;
    zone->low_vel = VEL_MIN;
    zone->high_vel = VEL_MAX;
    zone->amp = 1.0;
    zone->mute = 0;
    zone->solo = 0;
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

  inline int zone_contains(const jm::zone* zone, int pitch, int velocity) {
    return pitch >= zone->low_key && pitch <= zone->high_key &&
      velocity >= zone->low_vel && velocity <= zone->high_vel;
  }

  inline void build_zone_str(char* outstr, const std::vector<jm::zone>& zones, int i) {
    char* p = outstr;
    // index
    sprintf(p, "%i,", i);
    // wave length
    p += strlen(p);
    sprintf(p, "%i,", zones[i].wave_length);
    // name
    p += strlen(p);
    sprintf(p, "%s,", zones[i].name);
    // amp
    p += strlen(p);
    sprintf(p, "%f,", zones[i].amp);
    // mute
    p += strlen(p);
    sprintf(p, "%i,", zones[i].mute);
    // solo
    p += strlen(p);
    sprintf(p, "%i,", zones[i].solo);
    // origin
    p += strlen(p);
    sprintf(p, "%i,", zones[i].origin);
    // low key
    p += strlen(p);
    sprintf(p, "%i,", zones[i].low_key);
    // high key
    p += strlen(p);
    sprintf(p, "%i,", zones[i].high_key);
    // low vel
    p += strlen(p);
    sprintf(p, "%i,", zones[i].low_vel);
    // high vel
    p += strlen(p);
    sprintf(p, "%i,", zones[i].high_vel);
    // pitch
    p += strlen(p);
    sprintf(p, "%f,", zones[i].pitch_corr);
    // start
    p += strlen(p);
    sprintf(p, "%i,", zones[i].start);
    // left
    p += strlen(p);
    sprintf(p, "%i,", zones[i].left);
    // right
    p += strlen(p);
    sprintf(p, "%i,", zones[i].right);
    // loop mode
    p += strlen(p);
    sprintf(p, "%i,", zones[i].loop_mode);
    // crossfade
    p += strlen(p);
    sprintf(p, "%i,", zones[i].crossfade);
    // group
    p += strlen(p);
    sprintf(p, "%i,", zones[i].group);
    // off group
    p += strlen(p);
    sprintf(p, "%i,", zones[i].off_group);
    // attack
    p += strlen(p);
    sprintf(p, "%i,", zones[i].attack);
    // hold
    p += strlen(p);
    sprintf(p, "%i,", zones[i].hold);
    // decay
    p += strlen(p);
    sprintf(p, "%i,", zones[i].decay);
    // sustain
    p += strlen(p);
    sprintf(p, "%f,", zones[i].sustain);
    // release
    p += strlen(p);
    sprintf(p, "%i,", zones[i].release);
    // path
    p += strlen(p);
    sprintf(p, "%s\n", zones[i].path);
  }
};

#endif
