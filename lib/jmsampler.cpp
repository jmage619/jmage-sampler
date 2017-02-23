#include <cmath>
#include <cstdio>
#include <stdexcept> 

#include <vector>

#include "zone.h"
#include "collections.h"
#include "components.h"
#include "jmsampler.h"

// HACK until we figure out where to find this in LV2 instantiate!!
#define AUDIO_BUF_SIZE 4096

JMSampler::JMSampler(const std::vector<jm::zone>& zones):
    zones(zones),
    sustain_on(false),
    sound_gens(POLYPHONY),
    playhead_pool(POLYPHONY),
    amp_gen_pool(POLYPHONY) {
  // pre-allocate vector to prevent allocations later in RT thread
  //zones.reserve(100);

  for (size_t i = 0; i < POLYPHONY; ++i) {
    amp_gen_pool.push(new AmpEnvGenerator(amp_gen_pool));
    playhead_pool.push(new Playhead(playhead_pool, AUDIO_BUF_SIZE));
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
}

void JMSampler::pre_process(size_t nframes) {
  // pitch existing playheads
  for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
    sg_el->sg->pre_process(nframes);
  }
}

void JMSampler::handle_note_on(const char* midi_msg, size_t nframes, size_t curframe) {
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
  std::vector<jm::zone>::const_iterator it;
  for (it = zones.begin(); it != zones.end(); ++it) {
    if (jm::zone_contains(*it, midi_msg[1], midi_msg[2])) {
      fprintf(stderr, "sg num: %li\n", sound_gens.size());
      // oops we hit polyphony, remove oldest sound gen in the queue to make room
      if (sound_gens.size() >= POLYPHONY) {
        fprintf(stderr, "hit poly lim!\n");
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
      fprintf(stderr, "pre process start\n");
      ag->pre_process(nframes - curframe);
      fprintf(stderr, "pre process finish\n");

      // add sound gen to queue
      sound_gens.add(ag);
      fprintf(stderr, "event: channel: %i; note on;  note: %i; vel: %i\n", midi_msg[0] & 0x0F, midi_msg[1], midi_msg[2]);
    }
  }
}

void JMSampler::handle_note_off(const char* midi_msg) {
  fprintf(stderr, "event: note off; note: %i\n", midi_msg[1]);
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

void JMSampler::handle_sustain(const char* midi_msg) {
  // >= 64 turns on sustain
  if (midi_msg[2] >= 64) {
    sustain_on = true;
    fprintf(stderr, "sustain on\n");
  }
  // < 64 turns ustain off
  else {
    for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
      // turn off all sound gens marked with previous note off
      if (sg_el->sg->note_off == true)
        sg_el->sg->set_release();
    }

    sustain_on = false;
    fprintf(stderr, "sustain off\n");
  }
}

void JMSampler::process_frame(size_t curframe, float amp, float* out1, float* out2) {
  // loop sound gens and fill audio buffer at current time (frame) position
  for (sg_list_el* sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
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
