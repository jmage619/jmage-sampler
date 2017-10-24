#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <cmath>

#include <lib/zone.h>
#include <lib/sfzparser.h>
#include "jacksampler.h"

#define MSG_Q_SIZE 32

JackSampler::JackSampler(int sample_rate, size_t in_nframes, size_t out_nframes):
    JMSampler(sample_rate, in_nframes, out_nframes),
    _volume(16),
    _channel(0),
    msg_q(MSG_Q_SIZE) {

  volume = &_volume;
  channel = &_channel;
}

void JackSampler::send_add_zone(int index) {
  char outstr[256];
  char* p = outstr;
  sprintf(p, "add_zone:");
  p += strlen(p);
  jm::build_zone_str(p, zones, index);
  fprintf(fout, outstr);
  fflush(fout);
}

void JackSampler::send_update_wave(int index) {
  char outstr[256];
  char* p = outstr;
  sprintf(p, "update_wave:");
  // index
  p += strlen(p);
  sprintf(p, "%i,", index);
  // path
  p += strlen(p);
  sprintf(p, "%s,", zones[index].path);
  // wave length
  p += strlen(p);
  sprintf(p, "%i,", zones[index].wave_length);
  // start
  p += strlen(p);
  sprintf(p, "%i,", zones[index].start);
  // left
  p += strlen(p);
  sprintf(p, "%i,", zones[index].left);
  // right
  p += strlen(p);
  sprintf(p, "%i\n", zones[index].right);
  std::cout << outstr << std::endl;
  fprintf(fout, outstr);
  fflush(fout);
}
