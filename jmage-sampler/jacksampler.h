// jacksampler.h
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

#ifndef JACKSAMPLER_H
#define JACKSAMPLER_H

#include <cstdio>
#include <vector>
#include <map>
#include <pthread.h>

#include <jack/types.h>

#include <lib/zone.h>
#include <lib/wave.h>
#include <lib/collections.h>
#include <lib/sfzparser.h>
#include <lib/jmsampler.h>

enum msg_type {
MT_VOLUME,
MT_CHANNEL
};

struct jm_msg {
  msg_type type;
  union {
    int i;
    float f;
  } data;
};

class JackSampler: public JMSampler {
  public:
    jack_port_t* input_port;
    jack_port_t* output_port1;
    jack_port_t* output_port2;
    float _volume;
    float _channel;
    FILE* fout;
    JMQueue<jm_msg> msg_q;

    JackSampler(int sample_rate, size_t in_nframes, size_t out_nframes);
    void send_add_zone(int index);
    void send_update_wave(int index);
};

#endif
