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

#include <vector>
#include <fstream>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <iostream>
using std::cerr;
using std::endl;

#include <stdexcept> 

#include <pthread.h>

#include "zone.h"
#include "wave.h"
#include "sfzparser.h"
#include "collections.h"
#include "components.h"
#include "jmsampler.h"

JMSampler::JMSampler(int sample_rate, size_t in_nframes, size_t out_nframes):
    zone_number(1),
    sustain_on(false),
    solo_count(0),
    sound_gens(POLYPHONY),
    playhead_pool(POLYPHONY),
    amp_gen_pool(POLYPHONY),
    sample_rate(sample_rate) {
  // pre-allocate vector to prevent allocations later in RT thread
  zones.reserve(100);
  pthread_mutex_init(&zone_lock, NULL);

  for (size_t i = 0; i < POLYPHONY; ++i) {
    amp_gen_pool.push(new AmpEnvGenerator(amp_gen_pool));
    playhead_pool.push(new Playhead(playhead_pool, sample_rate, in_nframes, out_nframes));
  }
}

JMSampler::~JMSampler() {
  // clean up whatever is left in sg list
  while (sound_gens.size() > 0) {
    sound_gens.get_tail_ptr()->sg->release_resources();
    sound_gens.remove_last();
  }

  // then de-allocate sound generators
  while (playhead_pool.size() > 0)
    delete playhead_pool.pop();

  while (amp_gen_pool.size() > 0)
    delete amp_gen_pool.pop();

  std::map<std::string, jm::wave>::iterator it;
  for (it = waves.begin(); it != waves.end(); ++it)
    jm::free_wave(it->second);

  pthread_mutex_destroy(&zone_lock);
}

void JMSampler::add_zone_from_wave(int index, const char* path) {
  jm::wave& wav = waves[path];
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
  sprintf(zone.name, "Zone %i", zone_number++);
  strcpy(zone.path, path);
  if (index >= 0) {
    pthread_mutex_lock(&zone_lock);
    zones.insert(zones.begin() + index, zone);
    pthread_mutex_unlock(&zone_lock);
    send_add_zone(index);
  }
  else {
    pthread_mutex_lock(&zone_lock);
    zones.push_back(zone);
    pthread_mutex_unlock(&zone_lock);
    send_add_zone(zones.size() - 1);
  }
}

void JMSampler::add_zone_from_region(const std::map<std::string, SFZValue>& region) {
  jm::wave& wav = waves[region.find("sample")->second.get_str()];
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
    sprintf(zone.name, "Zone %i", zone_number++);

  it = region.find("jm_mute");
  zone.mute = it != region.end() ? it->second.get_int(): 0;
  it = region.find("jm_solo");
  zone.solo = it != region.end() ? it->second.get_int(): 0;
  if (zone.solo)
    ++solo_count;

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

  zone.crossfade = sample_rate * region.find("loop_crossfade")->second.get_double();
  zone.group = region.find("group")->second.get_int();
  zone.off_group = region.find("off_group")->second.get_int();
  zone.attack = sample_rate * region.find("ampeg_attack")->second.get_double();
  zone.hold = sample_rate * region.find("ampeg_hold")->second.get_double();
  zone.decay = sample_rate * region.find("ampeg_decay")->second.get_double();
  zone.sustain = region.find("ampeg_sustain")->second.get_double() / 100.;
  zone.release = sample_rate * region.find("ampeg_release")->second.get_double();
  pthread_mutex_lock(&zone_lock);
  zones.push_back(zone);
  pthread_mutex_unlock(&zone_lock);
}

void JMSampler::duplicate_zone(int index) {
  jm::zone zone;
  zone = zones[index];
  if (zones[index].solo)
    ++solo_count;

  pthread_mutex_lock(&zone_lock);
  zones.insert(zones.begin() + index + 1, zone);
  pthread_mutex_unlock(&zone_lock);
  send_add_zone(index + 1);
}

void JMSampler::remove_zone(int index) {
  pthread_mutex_lock(&zone_lock);
  std::vector<jm::zone>::iterator it = zones.begin() + index;
  if (it->solo)
    --solo_count;
  zones.erase(zones.begin() + index);
  pthread_mutex_unlock(&zone_lock);
}

void JMSampler::load_patch(const char* path) {
  SFZParser* parser;

  int len = strlen(path);
  if (!strcmp(path + len - 4, ".jmz"))
    parser = new JMZParser(path);
  // assumed it could ony eitehr be jmz or sfz
  else
    parser = new SFZParser(path);

  // consider error handling here
  patch = parser->parse();

  delete parser;

  zone_number = 1;
  solo_count = 0;

  pthread_mutex_lock(&zone_lock);
  zones.erase(zones.begin(), zones.end());
  pthread_mutex_unlock(&zone_lock);

  std::vector<std::map<std::string, SFZValue> >::iterator it;
  for (it = patch.regions.begin(); it != patch.regions.end(); ++it) {
    std::string wav_path = (*it)["sample"].get_str();
    if (waves.find(wav_path) == waves.end()) {
      waves[wav_path] = jm::parse_wave(wav_path.c_str());
    }
    add_zone_from_region(*it);
  }
}

void JMSampler::save_patch(const char* path) {
  int len = strlen(path);

  sfz::sfz save_patch;

  bool is_jmz = !strcmp(path + len - 4, ".jmz");
  if (is_jmz) {
    save_patch.control["jm_vol"] = (double) *volume;
    save_patch.control["jm_chan"] = (int) *channel + 1;
  }

  std::vector<jm::zone>::iterator it;
  for (it = zones.begin(); it != zones.end(); ++it) {
    std::map<std::string, SFZValue> region;

    if (is_jmz) {
      region["jm_name"] = it->name;
      region["jm_mute"] = it->mute;
      region["jm_solo"] = it->solo;
    }

    region["sample"] = it->path;
    region["volume"] = 20. * log10(it->amp);
    region["lokey"] = it->low_key;
    region["hikey"] = it->high_key;
    region["pitch_keycenter"] = it->origin;
    region["lovel"] = it->low_vel;
    region["hivel"] = it->high_vel;
    region["tune"] = (int) (100. * it->pitch_corr);
    region["offset"] = it->start;
    region["loop_mode"] = it->loop_mode;
    region["loop_start"] = it->left;
    region["loop_end"] = it->right;
    region["loop_crossfade"] = (double) it->crossfade / sample_rate;
    region["group"] = it->group;
    region["off_group"] = it->off_group;
    region["ampeg_attack"] = (double) it->attack / sample_rate;
    region["ampeg_hold"] = (double) it->hold / sample_rate;
    region["ampeg_decay"] = (double) it->decay / sample_rate;
    region["ampeg_sustain"] = 100. * it->sustain;
    region["ampeg_release"] = (double) it->release / sample_rate;

    save_patch.regions.push_back(region);
  }

  std::ofstream fout(path);

  sfz::write(&save_patch, fout);
  fout.close();
}

void JMSampler::reload_waves() {
  pthread_mutex_lock(&zone_lock);

  std::map<std::string, jm::wave>::iterator it;
  for (it = waves.begin(); it != waves.end(); ++it)
    jm::free_wave(it->second);

  waves.clear();

  std::vector<jm::zone>::const_iterator zit;
  for (zit = zones.begin(); zit != zones.end(); ++zit) {
    if (waves.find(zit->path) == waves.end()) {
      waves[zit->path] = jm::parse_wave(zit->path);
    }
  }

  pthread_mutex_unlock(&zone_lock);
}

void JMSampler::update_zone(int index, int key, const char* val) {
  pthread_mutex_lock(&zone_lock);
  switch (key) {
    case jm::ZONE_NAME:
      strcpy(zones[index].name, val);
      break;
    case jm::ZONE_AMP:
      zones[index].amp = atof(val);
      break;
    case jm::ZONE_MUTE:
      zones[index].mute = atoi(val);
      break;
    case jm::ZONE_SOLO:
      zones[index].solo = atoi(val);
      zones[index].solo ? ++solo_count: --solo_count;
      break;
    case jm::ZONE_ORIGIN:
      zones[index].origin = atoi(val);
      break;
    case jm::ZONE_LOW_KEY:
      zones[index].low_key = atoi(val);
      break;
    case jm::ZONE_HIGH_KEY:
      zones[index].high_key = atoi(val);
      break;
    case jm::ZONE_LOW_VEL:
      zones[index].low_vel = atoi(val);
      break;
    case jm::ZONE_HIGH_VEL:
      zones[index].high_vel = atoi(val);
      break;
    case jm::ZONE_PITCH:
      zones[index].pitch_corr = atof(val);
      break;
    case jm::ZONE_START:
      zones[index].start = atoi(val);
      break;
    case jm::ZONE_LEFT:
      zones[index].left = atoi(val);
      break;
    case jm::ZONE_RIGHT:
      zones[index].right = atoi(val);
      break;
    case jm::ZONE_LOOP_MODE:
      zones[index].loop_mode = (jm::loop_mode) atoi(val);
      break;
    case jm::ZONE_CROSSFADE:
      zones[index].crossfade = atoi(val);
      break;
    case jm::ZONE_GROUP:
      zones[index].group = atoi(val);
      break;
    case jm::ZONE_OFF_GROUP:
      zones[index].off_group = atoi(val);
      break;
    case jm::ZONE_ATTACK:
      zones[index].attack = atoi(val);
      break;
    case jm::ZONE_HOLD:
      zones[index].hold = atoi(val);
      break;
    case jm::ZONE_DECAY:
      zones[index].decay = atoi(val);
      break;
    case jm::ZONE_SUSTAIN:
      zones[index].sustain = atof(val);
      break;
    case jm::ZONE_RELEASE:
      zones[index].release = atoi(val);
      break;
    case jm::ZONE_PATH:
      jm::wave& wav = waves[val];
      zones[index].wave = wav.wave;
      zones[index].num_channels = wav.num_channels;
      zones[index].sample_rate = wav.sample_rate;
      zones[index].wave_length = wav.length;

      if (zones[index].start > wav.length)
        zones[index].start = wav.length;
      if (zones[index].left > wav.length)
        zones[index].left = wav.length;
      if (zones[index].right > wav.length)
        zones[index].right = wav.length;

      strcpy(zones[index].path, val);

      send_update_wave(index);
      break;
  }
  pthread_mutex_unlock(&zone_lock);
}

void JMSampler::pre_process(size_t nframes) {
  // pitch existing playheads
  for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
    sg_el->sg->pre_process(nframes);
  }
}

void JMSampler::handle_note_on(const unsigned char* midi_msg, size_t nframes, size_t curframe) {
  sg_list_el* sg_el;
  // if sustain on and note is already playing, release old one first
  if (sustain_on) {
    for (sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
      // doesn't apply to one shot
      if (!sg_el->sg->one_shot && sg_el->sg->pitch == midi_msg[1])
        sg_el->sg->set_release();
    }
  }
  // pick out zones midi event matches against and add sound gens to queue
  // yes, using mutex here violates the laws of real time ..BUT..
  // we don't expect a musician to tweak zones during an actual take!
  // this allows for demoing zone changes in thread safe way in *almost* real time
  // we can safely assume this mutex will be unlocked in a real take
  pthread_mutex_lock(&zone_lock);
  std::vector<jm::zone>::const_iterator it;
  for (it = zones.begin(); it != zones.end(); ++it) {
    if (jm::zone_contains(&*it, midi_msg[1], midi_msg[2]) && 
        (it->solo || (!solo_count && !it->mute))) {
      //cerr << "sg num: " << sound_gens.size() << endl;
      // oops we hit polyphony, remove oldest sound gen in the queue to make room
      if (sound_gens.size() >= POLYPHONY) {
        //cerr << "hit poly lim!" << endl;
        sound_gens.get_tail_ptr()->sg->release_resources();
        sound_gens.remove_last();
      }

      // shut off any sound gens that are in this off group
      if (it->group > 0) {
        for (sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
          if (sg_el->sg->off_group == it->group)
            sg_el->sg->set_release();
        }
      }

      // create sound gen
      AmpEnvGenerator* ag = amp_gen_pool.pop();
      Playhead* ph = playhead_pool.pop();
      ph->init(*it, midi_msg[1]);
      ag->init(ph, *it, midi_msg[1], midi_msg[2]);
      //cerr << "pre process start" << endl;
      ag->pre_process(nframes - curframe);
      //cerr << "pre process finish" << endl;

      // add sound gen to queue
      sound_gens.add(ag);
      //cerr << "event: channel: " << (midi_msg[0] & 0x0F) << "; note on;  note: " << midi_msg[1] << "; vel: " << midi_msg[2] << endl;
    }
  }
  pthread_mutex_unlock(&zone_lock);
}

void JMSampler::handle_note_off(const unsigned char* midi_msg) {
  //cerr << "event: note off; note: " << midi_msg[1] << endl;
  // find all sound gens assigned to this pitch
  for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
    if (sg_el->sg->pitch == midi_msg[1]) {
      // note off does not apply to one shot
      if (!sg_el->sg->one_shot) {
        // if sustaining, just mark for removal later
        if (sustain_on) {
          sg_el->sg->note_off = true;
        }
        // not sustaining, remove immediately
        else
          sg_el->sg->set_release();
      }
    }
  }
}

void JMSampler::handle_sustain(const unsigned char* midi_msg) {
  // >= 64 turns on sustain
  if (midi_msg[2] >= 64) {
    sustain_on = true;
    //cerr << "sustain on" << endl;
  }
  // < 64 turns ustain off
  else {
    for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
      // turn off all sound gens marked with previous note off
      if (sg_el->sg->note_off == true)
        sg_el->sg->set_release();
    }

    sustain_on = false;
    //cerr << "sustain off" << endl;
  }
}

void JMSampler::process_frame(size_t curframe, float* out1, float* out2) {
  // loop sound gens and fill audio buffer at current time (frame) position
  for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
    float amp = get_amp(*volume);
    float values[2];
    sg_el->sg->get_values(values);
    out1[curframe] += amp * values[0];
    out2[curframe] += amp * values[1];

    sg_el->sg->inc();
    if (sg_el->sg->is_finished()) {
      sg_el->sg->release_resources();
      sound_gens.remove(sg_el);
    }
  }
}
