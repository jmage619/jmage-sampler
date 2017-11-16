// jmage-sampler.cpp
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

#include <iostream>
using std::cerr;
using std::endl;

#include <vector>
#include <map>

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <pthread.h>

#include <jack/types.h>
#include <jack/jack.h>
#include <jack/midiport.h>

#include <config.h>
#include <lib/jmsampler.h>
#include <lib/wave.h>
#include <lib/zone.h>
#include <lib/collections.h>
#include <lib/sfzparser.h>

#include "jacksampler.h"

typedef jack_default_audio_sample_t sample_t;

int process_callback(jack_nframes_t nframes, void* arg) {
  JackSampler* sampler = static_cast<JackSampler*>(arg);
  sample_t* buffer1 = (sample_t*) jack_port_get_buffer(sampler->output_port1, nframes);
  sample_t* buffer2 = (sample_t*) jack_port_get_buffer(sampler->output_port2, nframes);
  memset(buffer1, 0, sizeof(sample_t) * nframes);
  memset(buffer2, 0, sizeof(sample_t) * nframes);

  // handle any UI messages
  while (!sampler->msg_q.empty()) {
    jm_msg msg = sampler->msg_q.remove();
    switch (msg.type) {
      case MT_VOLUME:
        *sampler->volume = msg.data.f;
        break;
      case MT_CHANNEL:
        *sampler->channel = msg.data.i;
        break;
      default:
        break;
    }
  }
  sampler->pre_process(nframes);

  // capture midi event
  void* midi_buf = jack_port_get_buffer(sampler->input_port, nframes);

  uint32_t event_count = jack_midi_get_event_count(midi_buf);
  uint32_t cur_event = 0;
  jack_midi_event_t event;
  if (event_count > 0)
    jack_midi_event_get(&event, midi_buf, cur_event);

  // loop over frames in this callback window
  for (jack_nframes_t n = 0; n < nframes; ++n) {
    if (cur_event < event_count) {
      // procces next midi event if it applies to current time (frame)
      while (n == event.time) {
        // only consider events from current channel
        if ((event.buffer[0] & 0x0f) == *sampler->channel) {
          // process note on
          if ((event.buffer[0] & 0xf0) == 0x90) {
            sampler->handle_note_on(event.buffer, nframes, n);
          }
          // process note off
          else if ((event.buffer[0] & 0xf0) == 0x80) {
            sampler->handle_note_off(event.buffer);
          }
          // process sustain pedal
          else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
            sampler->handle_sustain(event.buffer);
          }
          // just print messages we don't currently handle
          //else if (event.buffer[0] != 0xfe)
          //  printf("event: 0x%x\n", event.buffer[0]);
        }
        // get next midi event or break if none left
        ++cur_event;
        if (cur_event == event_count)
          break;
        jack_midi_event_get(&event, midi_buf, cur_event);
      }
    }
    sampler->process_frame(n, buffer1, buffer2);
  }

  return 0;
}

int main() {
  jack_client_t* client;

  // init jack
  jack_status_t status;
  if ((client = jack_client_open("jmage-sampler", JackNullOption, &status)) == NULL) {
    cerr << "failed to open jack client" << endl;
    return 1;
  }

  int sample_rate = jack_get_sample_rate(client);

  // supposed to also implement jack_set_buffer_size_callback; for now assume rarely changes
  jack_nframes_t jack_buf_size = jack_get_buffer_size(client);
  JackSampler* sampler = new JackSampler(sample_rate, jack_buf_size, jack_buf_size);

  jack_set_process_callback(client, process_callback, sampler);
  sampler->input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  sampler->output_port1 = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  sampler->output_port2 = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  if (jack_activate(client)) {
    jack_port_unregister(client, sampler->input_port);
    jack_port_unregister(client, sampler->output_port1);
    jack_port_unregister(client, sampler->output_port2);
    jack_client_close(client);
    delete sampler;
    cerr <<"cannot activate jack client" << endl;
    return 1;
  }
  
  int from_child_pipe[2];
  int to_child_pipe[2];

  pipe(from_child_pipe);
  pipe(to_child_pipe);

  pid_t pid = fork();

  // i'm the child
  if (pid == 0) {
    dup2(from_child_pipe[1], 1);
    close(from_child_pipe[0]);

    dup2(to_child_pipe[0], 0);
    close(to_child_pipe[1]);

    execl(CONFIG_INSTALL_PREFIX "/libexec/jm-sampler-ui", "jm-sampler-ui", NULL);
  }
  // i'm the parent
  close(from_child_pipe[1]);
  close(to_child_pipe[0]);

  FILE* fin = fdopen(from_child_pipe[0], "r");
  FILE* fout = fdopen(to_child_pipe[1], "w");
  sampler->fout = fout;

  char buf[256];

  fprintf(fout, "set_sample_rate:%i\n", sampler->sample_rate);
  fflush(fout);

  fprintf(fout, "update_vol:%f\n", *sampler->volume);
  fflush(fout);

  fprintf(fout, "update_chan:%i\n", (int) *sampler->channel);
  fflush(fout);

  while (fgets(buf, 256, fin) != NULL) {
    // kill newline char
    buf[strlen(buf) - 1] = '\0';
    //cerr << "UI: " << buf << endl;
    if (!strncmp(buf, "update_vol:", 11)) {
      jm_msg msg;
      msg.type = MT_VOLUME;
      msg.data.f = atof(buf + 11);

      sampler->msg_q.add(msg);
    }
    else if (!strncmp(buf, "update_chan:", 12)) {
      jm_msg msg;
      msg.type = MT_CHANNEL;
      msg.data.i = atoi(buf + 12);

      sampler->msg_q.add(msg);
    }
    else if (!strncmp(buf, "add_zone:", 9)) {
      char* p = strtok(buf + 9, ",");
      int index = atoi(p);
      p = strtok(NULL, ",");

      if (sampler->waves.find(p) == sampler->waves.end()) {
        sampler->waves[p] = jm::parse_wave(p);
      }

      sampler->add_zone_from_wave(index, p);
    }
    else if (!strncmp(buf, "dup_zone:", 9)) {
      char* p = strtok(buf + 9, ",");
      int index = atoi(p);
      sampler->duplicate_zone(index);
    }
    else if (!strncmp(buf, "remove_zone:", 12)) {
      int index = atoi(buf + 12);
      sampler->remove_zone(index);

      fprintf(fout, "remove_zone:%i\n", index);
      fflush(fout);
    }
    else if (!strncmp(buf, "update_zone:", 12)) {
      char* p = strtok(buf + 12, ",");
      int index = atoi(p);
      p = strtok(NULL, ",");
      int key = atoi(p);

      p = strtok(NULL, ",");

      // special case, update wave
      if (key == jm::ZONE_PATH) {
        if (sampler->waves.find(p) == sampler->waves.end()) {
          sampler->waves[p] = jm::parse_wave(p);
        }
      }

      sampler->update_zone(index, key, p);
    }
    else if (!strncmp(buf, "load_patch:", 11)) {
      sampler->load_patch(buf + 11);

      fprintf(fout, "clear_zones\n");
      fflush(fout);

      std::map<std::string, SFZValue>::iterator c_it = sampler->patch.control.find("jm_vol");
      if (c_it != sampler->patch.control.end()) {
        *sampler->volume = c_it->second.get_double();
        *sampler->channel =  sampler->patch.control["jm_chan"].get_int() - 1;
      }
      else {
        *sampler->volume = 0;
        *sampler->channel = 0;
      }

      fprintf(fout, "update_vol:%f\n", *sampler->volume);
      fflush(fout);

      fprintf(fout, "update_chan:%i\n", (int) *sampler->channel);
      fflush(fout);

      int num_zones = sampler->zones.size();

      for (int i = 0; i < num_zones; ++i)
        sampler->send_add_zone(i);
    }
    else if (!strncmp(buf, "save_patch:", 11)) {
      sampler->save_patch(buf + 11);
    }
    else if (!strncmp(buf, "refresh", 7)) {
      sampler->reload_waves();
    }
  }

  fclose(fout);
  waitpid(pid, NULL, 0);

  jack_deactivate(client);
  jack_port_unregister(client, sampler->input_port);
  jack_port_unregister(client, sampler->output_port1);
  jack_port_unregister(client, sampler->output_port2);
  jack_client_close(client);

  delete sampler;

  return 0;
}
