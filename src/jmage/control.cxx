#include <string.h>
#include <math.h>
#include <stdio.h>

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include "jmage/components.h"
#include "jmage/control.h"
#include "jmage/sampler.h"

#define WAV_OFF_Q_SIZE 10
#define MSG_Q_SIZE 32

namespace {
  double amp[VOL_STEPS];
  int level = VOL_STEPS - 1;

  int sustain_on = 0;
  PlayheadList playheads(WAV_OFF_Q_SIZE);
}

namespace ctrl {
  jack_port_t *input_port;
  jack_port_t *output_port1;
  jack_port_t *output_port2;
  JMQueue<jm_msg> msg_q_in(MSG_Q_SIZE);
  JMQueue<jm_msg> msg_q_out(MSG_Q_SIZE);

  void init_amp() {
    for (int i = 0; i < VOL_STEPS; i++) {
      amp[i]  = 1 / 100. * pow(10, 2 * i / (VOL_STEPS - 1.0));
    }
    amp[0] = 0.0;
  }

  int process_callback(jack_nframes_t nframes, void *arg) {
    sample_t *buffer1 = (sample_t *) jack_port_get_buffer (output_port1, nframes);
    sample_t *buffer2 = (sample_t *) jack_port_get_buffer (output_port2, nframes);

    memset (buffer1, 0, sizeof (jack_default_audio_sample_t) * nframes);
    memset (buffer2, 0, sizeof (jack_default_audio_sample_t) * nframes);

    // handle any UI messages
    jm_msg msg;
    while (msg_q_in.remove(msg)) {
      if (msg.type == MT_VOLUME) {
        level = *((int*)msg.data);
      }
      msg_q_out.add(msg);
    }

    // capture midi event
    void* midi_buf = jack_port_get_buffer(input_port, nframes);

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
            if (sustain_on) {
              for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
                if (pel->ph.pitch == event.buffer[1])
                  pel->ph.state = Playhead::RELEASED;
              }
            }
            int i;
            for (i = 0; i < jm_num_zones; i++) {
              if (jm_zone_contains(&jm_zones[i], event.buffer[1])) {
                Playhead ph(jm_zones[i], event.buffer[1], event.buffer[2]);

                if (playheads.size() >= WAV_OFF_Q_SIZE)
                  playheads.remove_last();

                playheads.add(ph);
                printf("event: note on;  note: %i; vel: %i; amp: %f\n", event.buffer[1], event.buffer[2], ph.amp);
              }
            }
          }
          else if ((event.buffer[0] & 0xf0) == 0x80) {
            printf("event: note off; note: %i\n", event.buffer[1]);
            for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
              if (pel->ph.pitch == event.buffer[1]) {
                if (sustain_on) {
                  pel->ph.state = Playhead::NOTE_OFF;
                }
                else
                  pel->ph.state = Playhead::RELEASED;
              }
            }
          }
          else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
            if (event.buffer[2] >= 64) {
              sustain_on = 1;
              printf("sustain on\n");
            }
            else {
              for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
                if (pel->ph.state == Playhead::NOTE_OFF)
                  pel->ph.state = Playhead::RELEASED;
              }

              sustain_on = 0;
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

      for (pel = playheads.get_head_ptr(); pel != NULL; pel = pel->next) {
        double values[2];
        pel->ph.get_values(values);
        buffer1[n] += amp[level] * values[0];
        buffer2[n] += amp[level] * values[1];

        pel->ph.inc();
        if (pel->ph.state == Playhead::FINISHED)
          playheads.remove(pel);
      }
    }
    return 0;
  }
}
