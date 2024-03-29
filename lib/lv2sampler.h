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

#ifndef LV2SAMPLER_H
#define LV2SAMPLER_H

#include <vector>
#include <map>
#include <pthread.h>
#include <cstdio>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#include "zone.h"
#include "lv2_uris.h"
#include "sfzparser.h"
#include "jmsampler.h"

class LV2Sampler: public JMSampler {
  public:
    const LV2_Atom_Sequence* control_port;
    LV2_Atom_Sequence* notify_port;
    float* out1;
    float* out2;
    LV2_URID_Map* map;
    jm::uris uris;
    LV2_Worker_Schedule* schedule;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame seq_frame;
    char patch_path[256];

    LV2Sampler(int sample_rate, size_t in_nframes, size_t out_nframes):
      JMSampler(sample_rate, in_nframes, out_nframes) {patch_path[0] = '\0';};
};

#endif
