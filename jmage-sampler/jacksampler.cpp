#include <iostream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <lib/zone.h>
#include <lib/sfzparser.h>
#include "jacksampler.h"

void build_zone_str(char* outstr, const std::vector<jm::zone>& zones, int i) {
  char* p = outstr;
  sprintf(p, "add_zone:");
  // index
  p += strlen(p);
  sprintf(p, "%i,", i);
  // wave length
  p += strlen(p);
  sprintf(p, "%i,", zones[i].wave_length);
  // name
  p += strlen(p);
  sprintf(p, "%s,", zones[i].name);
  std::cerr << "UI: adding ui zone!! " << zones[i].name << std::endl;
  // amp
  p += strlen(p);
  sprintf(p, "%f,", zones[i].amp);
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

void add_zone_from_region(jack_sampler* sampler, const std::map<std::string, SFZValue>& region) {
  jm::wave& wav = sampler->waves[region.find("sample")->second.get_str()];
  jm::zone zone;
  jm::init_zone(&zone);
  zone.wave = wav.wave;
  zone.num_channels = wav.num_channels;
  zone.sample_rate = wav.sample_rate;
  zone.wave_length = wav.length;
  zone.left = wav.left;
  zone.right = wav.length;
  if (wav.has_loop)
    zone.loop_mode = jm::LOOP_CONTINUOUS;

  std::map<std::string, SFZValue>::const_iterator it = region.find("jm_name");
  if (it != region.end())
    strcpy(zone.name, it->second.get_str().c_str());
  else
    sprintf(zone.name, "Zone %i", sampler->zone_number++);

  strcpy(zone.path, region.find("sample")->second.get_str().c_str());
  zone.amp = pow(10., region.find("volume")->second.get_double() / 20.);
  zone.low_key = region.find("lokey")->second.get_int();
  zone.high_key = region.find("hikey")->second.get_int();
  zone.origin = region.find("pitch_keycenter")->second.get_int();
  zone.low_vel = region.find("lovel")->second.get_int();
  zone.high_vel = region.find("hivel")->second.get_int();
  zone.pitch_corr = region.find("tune")->second.get_int() / 100.;
  zone.start = region.find("offset")->second.get_int();

  jm::loop_mode mode = (jm::loop_mode) region.find("loop_mode")->second.get_int();
  if (mode != jm::LOOP_UNSET)
    zone.loop_mode = mode;

  int loop_start = region.find("loop_start")->second.get_int();
  if (loop_start >= 0)
    zone.left = loop_start;

  int loop_end = region.find("loop_end")->second.get_int();
  if (loop_end >= 0)
    zone.right = loop_end;

  zone.crossfade = sampler->sample_rate * region.find("loop_crossfade")->second.get_double();
  zone.group = region.find("group")->second.get_int();
  zone.off_group = region.find("off_group")->second.get_int();
  zone.attack = sampler->sample_rate * region.find("ampeg_attack")->second.get_double();
  zone.hold = sampler->sample_rate * region.find("ampeg_hold")->second.get_double();
  zone.decay = sampler->sample_rate * region.find("ampeg_decay")->second.get_double();
  zone.sustain = region.find("ampeg_sustain")->second.get_double() / 100.;
  zone.release = sampler->sample_rate * region.find("ampeg_release")->second.get_double();
  sampler->zones.push_back(zone);
}
