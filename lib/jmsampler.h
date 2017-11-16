// jmsampler.h
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

#ifndef JMSAMPLER_H
#define JMSAMPLER_H

#include <pthread.h>
#include <cmath>

#include "zone.h"
#include "wave.h"
#include "sfzparser.h"
#include "collections.h"
#include "components.h"

#define POLYPHONY 10
#define VOL_STEPS 17

class JMSampler {
  private:
    int zone_number;
    // state
    bool sustain_on;
    int solo_count;
    SoundGenList sound_gens;

    JMStack<Playhead*> playhead_pool;
    JMStack<AmpEnvGenerator*> amp_gen_pool;

  public:
    int sample_rate;
    float* volume;
    float* channel;
    sfz::sfz patch;
    std::map<std::string, jm::wave> waves;
    std::vector<jm::zone> zones;
    pthread_mutex_t zone_lock;
    JMSampler(int sample_rate, size_t in_nframes, size_t out_nframes);
    virtual ~JMSampler();
    virtual void send_add_zone(int index) {};
    virtual void send_update_wave(int index) {};
    void add_zone_from_wave(int index, const char* path);
    void add_zone_from_region(const std::map<std::string, SFZValue>& region);
    void duplicate_zone(int index);
    void remove_zone(int index);
    void load_patch(const char* path);
    void save_patch(const char* path);
    void reload_waves();
    void update_zone(int index, int key, const char* val);
    void pre_process(size_t nframes);
    void handle_note_on(const unsigned char* midi_msg, size_t nframes, size_t curframe);
    void handle_note_off(const unsigned char* midi_msg);
    void handle_sustain(const unsigned char* midi_msg);
    void process_frame(size_t curframe, float* out1, float* out2);
};

inline float get_amp(float index) {
  return powf(10.f, index / 20.f);
}

#endif
