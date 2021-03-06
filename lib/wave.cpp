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

#include <stdexcept>
#include <sstream>
#include <iostream>
using std::cerr;
using std::endl;

#include <sndfile.h>

#include "wave.h"

jm::wave jm::parse_wave(const char* path) {
  SF_INFO sf_info;
  sf_info.format = 0;
  SNDFILE* sf_wav = sf_open(path, SFM_READ, &sf_info);
  if (sf_wav == NULL) {
    std::ostringstream msg;
    msg << "error opening " << path << ": " << sf_strerror(NULL);
    throw std::runtime_error(msg.str());
  }

  wave wav;
  wav.length = sf_info.frames;
  wav.num_channels = sf_info.channels;
  wav.sample_rate = sf_info.samplerate;
  
  wav.wave = new float[wav.num_channels * wav.length];
  sf_read_float(sf_wav, wav.wave, wav.num_channels * wav.length);

  wav.has_loop = 0;
  wav.left = 0;
  wav.right = wav.length;

  SF_INSTRUMENT inst;
  if (sf_command(sf_wav, SFC_GET_INSTRUMENT, &inst, sizeof(inst)) == SF_FALSE) {
    //cerr << "wav " << path << ": no instrument info found, assuming 0 loop points" << endl;
    sf_close(sf_wav);
    return wav;
  } 

  // if the wav has loops, just pick its first one(?)
  // also ignoring loop mode and count for now
  if (inst.loop_count > 0) {
    wav.has_loop = 1;
    wav.left = inst.loops[0].start;
    wav.right = inst.loops[0].end;
  }

  sf_close(sf_wav);
  return wav;
}

