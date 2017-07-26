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

JackSampler::JackSampler(int sample_rate, size_t in_nframes, size_t out_nframes):
    JMSampler(sample_rate, in_nframes, out_nframes),
    _volume(16),
    _channel(0) {

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
