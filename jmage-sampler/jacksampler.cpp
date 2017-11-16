// jacksampler.cpp
//
/****************************************************************************
    Copyright (C) 2017  jmage619

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

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
    _volume(0),
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
  fprintf(fout, outstr);
  fflush(fout);
}
