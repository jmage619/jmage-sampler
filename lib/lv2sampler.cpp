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

#include <cstdio>
#include <cmath>
#include <map>
#include <vector>
#include <pthread.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>

#include "zone.h"
#include "wave.h"
#include "sfzparser.h"
#include "lv2_uris.h"
#include "lv2sampler.h"

void LV2Sampler::send_add_zone(int index) {
  char outstr[256];
  char* p = outstr;
  sprintf(p, "add_zone:");
  p += strlen(p);
  jm::build_zone_str(p, zones, index);
  fprintf(fout, outstr);
  fflush(fout);

  //fprintf(stderr, "SAMPLER: add zone sent!! %i: %s\n", index, zones[index].name);
}

void LV2Sampler::send_update_wave(int index) {
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

  //fprintf(stderr, "SAMPLER: update wave sent!! %i: %s\n", index, zones[index].path);
}
