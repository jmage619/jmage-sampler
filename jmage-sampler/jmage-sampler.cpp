#include <iostream>
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

#define MSG_Q_SIZE 32
#define VOL_STEPS 17

typedef jack_default_audio_sample_t sample_t;

float get_amp(int index) {
  return index == 0 ? 0.f: 1 / 100.f * pow(10.f, 2 * index / (VOL_STEPS - 1.0f));
}

int process_callback(jack_nframes_t nframes, void* arg) {
  jack_sampler* jsampler = static_cast<jack_sampler*>(arg);
  sample_t* buffer1 = (sample_t*) jack_port_get_buffer(jsampler->output_port1, nframes);
  sample_t* buffer2 = (sample_t*) jack_port_get_buffer(jsampler->output_port2, nframes);
  memset(buffer1, 0, sizeof(sample_t) * nframes);
  memset(buffer2, 0, sizeof(sample_t) * nframes);

  // handle any UI messages
  while (!jsampler->msg_q->empty()) {
    jm_msg msg = jsampler->msg_q->remove();
    switch (msg.type) {
      case MT_VOLUME:
        jsampler->volume = msg.data.i;
        break;
      case MT_CHANNEL:
        jsampler->channel = msg.data.i;
        break;
      default:
        break;
    }
  }
  jsampler->sampler->pre_process(nframes);

  // capture midi event
  void* midi_buf = jack_port_get_buffer(jsampler->input_port, nframes);

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
        if ((event.buffer[0] & 0x0f) == jsampler->channel) {
          // process note on
          if ((event.buffer[0] & 0xf0) == 0x90) {
            // yes, using mutex here violates the laws of real time ..BUT..
            // we don't expect a musician to tweak zones during an actual take!
            // this allows for demoing zone changes in thread safe way in *almost* real time
            // we can safely assume this mutex will be unlocked in a real take
            pthread_mutex_lock(&jsampler->zone_lock);
            jsampler->sampler->handle_note_on(event.buffer, nframes, n);
            pthread_mutex_unlock(&jsampler->zone_lock);
          }
          // process note off
          else if ((event.buffer[0] & 0xf0) == 0x80) {
            jsampler->sampler->handle_note_off(event.buffer);
          }
          // process sustain pedal
          else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
            jsampler->sampler->handle_sustain(event.buffer);
          }
          // just print messages we don't currently handle
          else if (event.buffer[0] != 0xfe)
            printf("event: 0x%x\n", event.buffer[0]);
        }
        // get next midi event or break if none left
        ++cur_event;
        if (cur_event == event_count)
          break;
        jack_midi_event_get(&event, midi_buf, cur_event);
      }
    }
    jsampler->sampler->process_frame(n, get_amp(jsampler->volume), buffer1, buffer2);
  }

  return 0;
}

int main() {
  jack_client_t* client;

  // init jack
  jack_status_t status;
  if ((client = jack_client_open("jmage-sampler", JackNullOption, &status)) == NULL) {
    std::cerr << "failed to open jack client" << std::endl;
    return 1;
  }

  jack_sampler* jsampler = new jack_sampler;

  jack_set_process_callback(client, process_callback, jsampler);
  jsampler->input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  jsampler->output_port1 = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  jsampler->output_port2 = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  jsampler->zones.reserve(100);

  pthread_mutex_init(&jsampler->zone_lock, NULL);

  jsampler->msg_q = new JMQueue<jm_msg>(MSG_Q_SIZE);

  jsampler->sample_rate = jack_get_sample_rate(client);
  // supposed to also implement jack_set_buffer_size_callback; for now assume rarely changes
  jack_nframes_t jack_buf_size = jack_get_buffer_size(client);
  jsampler->sampler = new JMSampler(jsampler->zones, jsampler->sample_rate, jack_buf_size, jack_buf_size);

  jsampler->zone_number = 1;
  jsampler->volume = 16;
  jsampler->channel = 0;

  if (jack_activate(client)) {
    jack_port_unregister(client, jsampler->input_port);
    jack_port_unregister(client, jsampler->output_port1);
    jack_port_unregister(client, jsampler->output_port2);
    jack_client_close(client);
    delete jsampler->sampler;
    delete jsampler->msg_q;
    pthread_mutex_destroy(&jsampler->zone_lock);
    delete jsampler;
    std::cerr <<"cannot activate jack client" << std::endl;
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

  char buf[256];

  fprintf(fout, "set_sample_rate:%i\n", jsampler->sample_rate);
  fflush(fout);

  fprintf(fout, "update_vol:%i\n", jsampler->volume);
  fflush(fout);

  fprintf(fout, "update_chan:%i\n", jsampler->channel);
  fflush(fout);

  while (fgets(buf, 256, fin) != NULL) {
    // kill newline char
    buf[strlen(buf) - 1] = '\0';
    std::cout << "UI: " << buf << std::endl;
    if (!strncmp(buf, "update_vol:", 11)) {
      jm_msg msg;
      msg.type = MT_VOLUME;
      msg.data.i = atoi(buf + 11);

      jsampler->msg_q->add(msg);
    }
    else if (!strncmp(buf, "update_chan:", 12)) {
      jm_msg msg;
      msg.type = MT_CHANNEL;
      msg.data.i = atoi(buf + 12);

      jsampler->msg_q->add(msg);
    }
    else if (!strncmp(buf, "add_zone:", 9)) {
      char* p = strtok(buf + 9, ",");
      int index = atoi(p);
      p = strtok(NULL, ",");

      if (jsampler->waves.find(p) == jsampler->waves.end()) {
        jsampler->waves[p] = jm::parse_wave(p);
      }

      jm::wave& wav = jsampler->waves[p];
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
      sprintf(zone.name, "Zone %i", jsampler->zone_number++);
      strcpy(zone.path, p);

      char outstr[256];
      if (index < 0) {
        pthread_mutex_lock(&jsampler->zone_lock);
        jsampler->zones.push_back(zone);
        pthread_mutex_unlock(&jsampler->zone_lock);
        build_zone_str(outstr, jsampler->zones, jsampler->zones.size() - 1);
      }
      else {
        pthread_mutex_lock(&jsampler->zone_lock);
        jsampler->zones.insert(jsampler->zones.begin() + index, zone);
        pthread_mutex_unlock(&jsampler->zone_lock);
        build_zone_str(outstr, jsampler->zones, index);
      }

      fprintf(fout, outstr);
      fflush(fout);
    }
    else if (!strncmp(buf, "remove_zone:", 12)) {
      int index = atoi(buf + 12);
      pthread_mutex_lock(&jsampler->zone_lock);
      jsampler->zones.erase(jsampler->zones.begin() + index);
      pthread_mutex_unlock(&jsampler->zone_lock);

      fprintf(fout, "remove_zone:%i\n", index);
      fflush(fout);
    }
    else if (!strncmp(buf, "update_zone:", 12)) {
      char* p = strtok(buf + 12, ",");
      int index = atoi(p);
      p = strtok(NULL, ",");
      int key = atoi(p);

      p = strtok(NULL, ",");

      pthread_mutex_lock(&jsampler->zone_lock);
      switch (key) {
        case jm::ZONE_NAME:
          strcpy(jsampler->zones[index].name, p);
          break;
        case jm::ZONE_AMP:
          jsampler->zones[index].amp = atof(p);
          break;
        case jm::ZONE_ORIGIN:
          jsampler->zones[index].origin = atoi(p);
          break;
        case jm::ZONE_LOW_KEY:
          jsampler->zones[index].low_key = atoi(p);
          break;
        case jm::ZONE_HIGH_KEY:
          jsampler->zones[index].high_key = atoi(p);
          break;
        case jm::ZONE_LOW_VEL:
          jsampler->zones[index].low_vel = atoi(p);
          break;
        case jm::ZONE_HIGH_VEL:
          jsampler->zones[index].high_vel = atoi(p);
          break;
        case jm::ZONE_PITCH:
          jsampler->zones[index].pitch_corr = atof(p);
          break;
        case jm::ZONE_START:
          jsampler->zones[index].start = atoi(p);
          break;
        case jm::ZONE_LEFT:
          jsampler->zones[index].left = atoi(p);
          break;
        case jm::ZONE_RIGHT:
          jsampler->zones[index].right = atoi(p);
          break;
        case jm::ZONE_LOOP_MODE:
          jsampler->zones[index].loop_mode = (jm::loop_mode) atoi(p);
          break;
        case jm::ZONE_CROSSFADE:
          jsampler->zones[index].crossfade = atoi(p);
          break;
        case jm::ZONE_GROUP:
          jsampler->zones[index].group = atoi(p);
          break;
        case jm::ZONE_OFF_GROUP:
          jsampler->zones[index].off_group = atoi(p);
          break;
        case jm::ZONE_ATTACK:
          jsampler->zones[index].attack = atoi(p);
          break;
        case jm::ZONE_HOLD:
          jsampler->zones[index].hold = atoi(p);
          break;
        case jm::ZONE_DECAY:
          jsampler->zones[index].decay = atoi(p);
          break;
        case jm::ZONE_SUSTAIN:
          jsampler->zones[index].sustain = atof(p);
          break;
        case jm::ZONE_RELEASE:
          jsampler->zones[index].release = atoi(p);
          break;
      }
      pthread_mutex_unlock(&jsampler->zone_lock);
    }
    else if (!strncmp(buf, "load_patch:", 11)) {
      char* path = buf + 11;
      SFZParser* parser;
      int len = strlen(path);
      if (!strcmp(path + len - 4, ".jmz"))
        parser = new JMZParser(path);
      // assumed it could ony eitehr be jmz or sfz
      else
        parser = new SFZParser(path);

      sfz::sfz patch;

      try {
        patch = parser->parse();
      }
      catch (std::runtime_error& e) {
        std::cerr << "failed parsing patch:" << e.what() << std::endl;
        delete parser;
        continue;
      }

      fprintf(fout, "clear_zones\n");
      fflush(fout);

      jsampler->zone_number = 1;

      std::map<std::string, SFZValue>::iterator c_it = patch.control.find("jm_vol");
      if (c_it != patch.control.end()) {
        jsampler->volume = c_it->second.get_int();
        jsampler-> channel =  patch.control["jm_chan"].get_int() - 1;
      }
      else {
        jsampler->volume = 16;
        jsampler->channel = 0;
      }

      fprintf(fout, "update_vol:%i\n", jsampler->volume);
      fflush(fout);

      fprintf(fout, "update_chan:%i\n", jsampler->channel);
      fflush(fout);

      
      pthread_mutex_lock(&jsampler->zone_lock);
      jsampler->zones.erase(jsampler->zones.begin(), jsampler->zones.end());
      pthread_mutex_unlock(&jsampler->zone_lock);

      std::vector<std::map<std::string, SFZValue>>::iterator it;
      char outstr[256];
      for (it = patch.regions.begin(); it != patch.regions.end(); ++it) {
        std::string wave_path = (*it)["sample"].get_str();
        if (jsampler->waves.find(wave_path) == jsampler->waves.end())
          jsampler->waves[wave_path] = jm::parse_wave(wave_path.c_str());

        pthread_mutex_lock(&jsampler->zone_lock);
        add_zone_from_region(jsampler, *it);      
        pthread_mutex_unlock(&jsampler->zone_lock);

        build_zone_str(outstr, jsampler->zones, jsampler->zones.size() - 1);
        fprintf(fout, outstr);
        fflush(fout);
      }

      delete parser;
    }
    else if (!strncmp(buf, "save_patch:", 11)) {
      char* path = buf + 11;
      save_patch(jsampler, path);
    }
  }

  fclose(fout);
  waitpid(pid, NULL, 0);

  jack_deactivate(client);
  jack_port_unregister(client, jsampler->input_port);
  jack_port_unregister(client, jsampler->output_port1);
  jack_port_unregister(client, jsampler->output_port2);
  jack_client_close(client);

  std::map<std::string, jm::wave>::iterator it;
  for (it = jsampler->waves.begin(); it != jsampler->waves.end(); ++it)
    jm::free_wave(it->second);

  delete jsampler->sampler;
  delete jsampler->msg_q;
  pthread_mutex_destroy(&jsampler->zone_lock);
  delete jsampler;

  return 0;
}
