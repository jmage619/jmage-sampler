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

JMSampler::JMSampler(std::vector<jm::zone>& zones):
    zones(zones),
    sustain_on(false),
    sound_gens(POLYPHONY),
    playhead_pool(POLYPHONY),
    amp_gen_pool(POLYPHONY) {
  // pre-allocate vector to prevent allocations later in RT thread
  //zones.reserve(100);

  for (size_t i = 0; i < POLYPHONY; ++i) {
    amp_gen_pool.push(new AmpEnvGenerator(&amp_gen_pool));
    playhead_pool.push(new Playhead(&playhead_pool, AUDIO_BUF_SIZE));
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
  sg_list_el* sg_el;
  for (sg_el = sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
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
  std::vector<jm::zone>::iterator it;
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

/*int JMSampler::process_callback(jack_nframes_t nframes, void* arg) {
  JMSampler* jms = (JMSampler*) arg;
  sample_t* buffer1 = (sample_t*) jack_port_get_buffer(jms->output_port1, nframes);
  sample_t* buffer2 = (sample_t*) jack_port_get_buffer(jms->output_port2, nframes);

  memset(buffer1, 0, sizeof(sample_t) * nframes);
  memset(buffer2, 0, sizeof(sample_t) * nframes);

  // handle any UI messages
  jm_msg msg;
  while (jms->msg_q_in.remove(msg)) {
    switch (msg.type) {
      case MT_VOLUME:
        *jms->level = msg.data.i;
        break;
      case MT_CHANNEL:
        *jms->channel = msg.data.i;
        break;
      default:
        break;
    }
  }

  // pitch existing playheads
  sg_list_el* sg_el;
  for (sg_el = jms->sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
    sg_el->sg->pre_process(nframes);
  }
  // capture midi event
  void* midi_buf = jack_port_get_buffer(jms->input_port, nframes);

  uint32_t event_count = jack_midi_get_event_count(midi_buf);
  uint32_t cur_event = 0;
  jack_midi_event_t event;
  if (event_count > 0)
    jack_midi_event_get(&event, midi_buf, cur_event);

  // loop over frames in this callback window
  jack_nframes_t n;
  for (n = 0; n < nframes; n++) {
    if (cur_event < event_count) {
      // procces next midi event if it applies to current time (frame)
      while (n == event.time) {
        // only consider events from current channel
        if ((event.buffer[0] & 0x0f) == (int) *jms->channel) {
          // process note on
          if ((event.buffer[0] & 0xf0) == 0x90) {
            // if sustain on and note is already playing, release old one first
            if (jms->sustain_on) {
              for (sg_el = jms->sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
                // doesn't apply to one shot
                if (!sg_el->sg->one_shot && sg_el->sg->pitch == event.buffer[1])
                  sg_el->sg->set_release();
              }
            }
            std::vector<jm_key_zone>::iterator it;
            // yes, using mutex here violates the laws of real time ..BUT..
            // we don't expect a musician to tweak zones during an actual take!
            // this allows for demoing zone changes in thread safe way in *almost* real time
            // we can safely assume this mutex will be unlocked in a real take
            //jms->zones->lock();
            // pick out zones midi event matches against and add sound gens to queue
            for (it = jms->zones.begin(); it != jms->zones.end(); ++it) {
              if (jm_zone_contains(&*it, event.buffer[1], event.buffer[2])) {
                printf("sg num: %li\n", jms->sound_gens.size());
                // oops we hit polyphony, remove oldest sound gen in the queue to make room
                if (jms->sound_gens.size() >= POLYPHONY) {
                  printf("hit poly lim!\n");
                  jms->sound_gens.get_tail_ptr()->sg->release_resources();
                  jms->sound_gens.remove_last();
                }

                // shut off any sound gens that are in this off group
                if (it->group > 0) {
                  for (sg_el = jms->sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
                    if (sg_el->sg->off_group == it->group)
                      sg_el->sg->set_release();
                  }
                }

                // create sound gen
                AmpEnvGenerator* ag;
                jms->amp_gen_pool.pop(ag);
                Playhead* ph;
                jms->playhead_pool.pop(ph);
                ph->init(*it, event.buffer[1]);
                ag->init(ph, *it, event.buffer[1], event.buffer[2]);
                printf("pre process start\n");
                ag->pre_process(nframes - n);
                printf("pre process finish\n");

                // add sound gen to queue
                jms->sound_gens.add(ag);
                printf("event: channel: %i; note on;  note: %i; vel: %i\n", event.buffer[0] & 0x0F, event.buffer[1], event.buffer[2]);
              }
            }
            //jms->zones->unlock();
          }
          // process note off
          else if ((event.buffer[0] & 0xf0) == 0x80) {
            printf("event: note off; note: %i\n", event.buffer[1]);
            // find all sound gens assigned to this pitch
            for (sg_el = jms->sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
              if (sg_el->sg->pitch == event.buffer[1]) {
                // note off does not apply to one shot
                if (!sg_el->sg->one_shot) {
                  // if sustaining, just mark for removal later
                  if (jms->sustain_on) {
                    sg_el->sg->note_off = true;
                  }
                  // not sustaining, remove immediately
                  else
                    sg_el->sg->set_release();
                }
              }
            }
          }
          // process sustain pedal
          else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
            // >= 64 turns on sustain
            if (event.buffer[2] >= 64) {
              jms->sustain_on = true;
              printf("sustain on\n");
            }
            // < 64 turns ustain off
            else {
              for (sg_el = jms->sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
                // turn off all sound gens marked with previous note off
                if (sg_el->sg->note_off == true)
                  sg_el->sg->set_release();
              }

              jms->sustain_on = false;
              printf("sustain off\n");
            }
          }
          // just print messages we don't currently handle
          else if (event.buffer[0] != 0xfe)
            printf("event: 0x%x\n", event.buffer[0]);
        }

        // get next midi event or break if none left
        cur_event++;
        if (cur_event == event_count)
          break;
        jack_midi_event_get(&event, midi_buf, cur_event);
      }
    }

    // loop sound gens and fill audio buffer at current time (frame) position
    for (sg_el = jms->sound_gens.get_head_ptr(); sg_el != NULL; sg_el = sg_el->next) {
      float values[2];
      sg_el->sg->get_values(values);
      buffer1[n] += jms->amp[(size_t) *jms->level] * values[0];
      buffer2[n] += jms->amp[(size_t) *jms->level] * values[1];

      sg_el->sg->inc();
      if (sg_el->sg->is_finished()) {
        sg_el->sg->release_resources();
        jms->sound_gens.remove(sg_el);
      }
    }
  }
  return 0;
}
*/
