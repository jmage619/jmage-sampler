#include <cstring>
#include <cmath>
#include <cstdio>
#include <stdexcept> 

#include <vector>

#include <pthread.h>
#include <jack/types.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "jmage/jmsampler.h"
#include "jmage/sampler.h"
#include "jmage/components.h"
#include "jmage/collections.h"

void JMSampler::init_amp(JMSampler* jms) {
  for (int i = 0; i < VOL_STEPS; i++) {
    jms->amp[i]  = 1 / 100. * pow(10, 2 * i / (VOL_STEPS - 1.0));
  }
  jms->amp[0] = 0.0;
}

JMSampler::JMSampler():
    msg_q_in(MSG_Q_SIZE),
    msg_q_out(MSG_Q_SIZE),
    level(VOL_STEPS - 1),
    sustain_on(false),
    playheads(MAX_PLAYHEADS),
    playhead_pool(MAX_PLAYHEADS) {
  // init amplitude array
  init_amp(this);

  // init zone map lock
  pthread_mutex_init(&zone_lock, NULL);

  // init jack
  jack_status_t status; 
  if ((client = jack_client_open("ghetto_sampler", JackNullOption, &status)) == NULL) {
    pthread_mutex_destroy(&zone_lock);
    throw std::runtime_error("failed to open jack client");
  }

  jack_set_process_callback(client, process_callback, this);
  input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  output_port1 = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  output_port2 = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  jack_buf_size = jack_get_buffer_size(client);
  for (size_t i = 0; i < MAX_PLAYHEADS; i += 1)
    playhead_pool.push(new Playhead(&playhead_pool, jack_buf_size));

  if (jack_activate(client)) {
    jack_port_unregister(client, input_port);
    jack_port_unregister(client, output_port1);
    jack_port_unregister(client, output_port2);
    jack_client_close(client);
    pthread_mutex_destroy(&zone_lock);
    throw std::runtime_error("cannot activate jack client");
  }
}

JMSampler::~JMSampler() {
  jack_deactivate(client);
  jack_port_unregister(client, input_port);
  jack_port_unregister(client, output_port1);
  jack_port_unregister(client, output_port2);
  jack_client_close(client);

  // clean up whatever is left in ph list
  while (playheads.size() > 0) {
    playheads.get_tail_ptr()->ph->release_resources();
    playheads.remove_last();
  }

  // then de-allocate playheads
  Playhead* ph;
  while (playhead_pool.pop(ph)) {
    delete ph;
  }
  pthread_mutex_destroy(&zone_lock);
}

void JMSampler::add_zone(const jm_key_zone& zone) {
  pthread_mutex_lock(&zone_lock);
  zones.push_back(zone);
  pthread_mutex_unlock(&zone_lock);
}

const jm_key_zone& JMSampler::get_zone(int index) {
  pthread_mutex_lock(&zone_lock);
  jm_key_zone& zone = zones[index];
  pthread_mutex_unlock(&zone_lock);
  return zone;
}

void JMSampler::update_zone(int index, const jm_key_zone& zone) {
  pthread_mutex_lock(&zone_lock);
  zones[index] = zone;
  pthread_mutex_unlock(&zone_lock);
}

void JMSampler::remove_zone(int index) {
  pthread_mutex_lock(&zone_lock);
  zones.erase(zones.begin() + index);
  pthread_mutex_unlock(&zone_lock);
}

size_t JMSampler::num_zones() {
  pthread_mutex_lock(&zone_lock);
  size_t size = zones.size();
  pthread_mutex_unlock(&zone_lock);
  return size;
}

void JMSampler::send_msg(const jm_msg& msg) {
  msg_q_in.add(msg);
}

bool JMSampler::receive_msg(jm_msg& msg) {
  return msg_q_out.remove(msg);
}

int JMSampler::process_callback(jack_nframes_t nframes, void* arg) {
  JMSampler* jms = (JMSampler*) arg;
  sample_t* buffer1 = (sample_t*) jack_port_get_buffer(jms->output_port1, nframes);
  sample_t* buffer2 = (sample_t*) jack_port_get_buffer(jms->output_port2, nframes);

  memset(buffer1, 0, sizeof(jack_default_audio_sample_t) * nframes);
  memset(buffer2, 0, sizeof(jack_default_audio_sample_t) * nframes);

  // handle any UI messages
  jm_msg msg;
  while (jms->msg_q_in.remove(msg)) {
    if (msg.type == MT_VOLUME) {
      jms->level = msg.data.i;
    }
  }

  // capture midi event
  void* midi_buf = jack_port_get_buffer(jms->input_port, nframes);

  uint32_t event_count = jack_midi_get_event_count(midi_buf);
  uint32_t cur_event = 0;
  jack_midi_event_t event;
  if (event_count > 0)
    jack_midi_event_get(&event, midi_buf, cur_event);

  ph_list_el* pel;
  jack_nframes_t n;
  for (n = 0; n <= nframes; n++) {
    if (cur_event < event_count) {
      while (n == event.time) {
        if ((event.buffer[0] & 0xf0) == 0x90) {
          if (jms->sustain_on) {
            for (pel = jms->playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
              if (pel->ph->pitch == event.buffer[1])
                pel->ph->set_release();
            }
          }
          std::vector<jm_key_zone>::iterator it;
          // yes, using mutex here violates the laws of real time ..BUT..
          // we don't expect a musician to tweak zones during an actual take!
          // this allows for demoing zone changes in thread safe way in *almost* real time
          // we can safely assume this mutex will be unlocked in a real take
          pthread_mutex_lock(&jms->zone_lock);
          for (it = jms->zones.begin(); it != jms->zones.end(); ++it) {
            if (jm_zone_contains(&*it, event.buffer[1])) {
              Playhead* ph;
              jms->playhead_pool.pop(ph);
              ph->init(*it, event.buffer[1], event.buffer[2]);

              if (jms->playheads.size() >= MAX_PLAYHEADS) {
                jms->playheads.get_tail_ptr()->ph->release_resources();
                jms->playheads.remove_last();
              }

              jms->playheads.add(ph);
              printf("event: note on;  note: %i; vel: %i; amp: %f\n", event.buffer[1], event.buffer[2], ph->amp);
            }
          }
          pthread_mutex_unlock(&jms->zone_lock);
        }
        else if ((event.buffer[0] & 0xf0) == 0x80) {
          printf("event: note off; note: %i\n", event.buffer[1]);
          for (pel = jms->playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
            if (pel->ph->pitch == event.buffer[1]) {
              if (jms->sustain_on) {
                pel->ph->note_off = true;
              }
              else
                pel->ph->set_release();
            }
          }
        }
        else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
          if (event.buffer[2] >= 64) {
            jms->sustain_on = true;
            printf("sustain on\n");
          }
          else {
            for (pel = jms->playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
              if (pel->ph->note_off == true)
                pel->ph->set_release();
            }

            jms->sustain_on = false;
            printf("sustain off\n");
          }
        }
        else if (event.buffer[0] != 0xfe)
          printf("event: 0x%x\n", event.buffer[0]);

        cur_event++;
        if (cur_event == event_count)
          break;
        jack_midi_event_get(&event, midi_buf, cur_event);
      }
    }

    for (pel = jms->playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
      double values[2];
      pel->ph->get_values(values);
      buffer1[n] += jms->amp[jms->level] * values[0];
      buffer2[n] += jms->amp[jms->level] * values[1];

      pel->ph->inc();
      if (pel->ph->state == Playhead::FINISHED) {
        pel->ph->release_resources();
        jms->playheads.remove(pel);
      }
    }
  }
  return 0;
}
