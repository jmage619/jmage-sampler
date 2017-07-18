#include <iostream>
#include <vector>
#include <map>

#include <cstring>
#include <cstdlib>
#include <cstdio>
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

typedef jack_default_audio_sample_t sample_t;

struct client_data {
  std::map<std::string, jm::wave> waves;
  std::vector<jm::zone> zones;
  pthread_mutex_t zone_lock;
  jack_port_t* input_port;
  jack_port_t* output_port1;
  jack_port_t* output_port2;
  JMSampler* sampler;
  int zone_number;
  int channel;
};

void build_zone_str(char* outstr, const std::vector<jm::zone>& zones, int i) {
  char* p = outstr;
  sprintf(p, "add_zone:");
  // index
  p += strlen(p);
  sprintf(p, "%i,", i);
  // wave length
  p += strlen(p);
  sprintf(p, "%i,", zones[i].wave_length);
  // name
  p += strlen(p);
  sprintf(p, "%s,", zones[i].name);
  std::cerr << "UI: adding ui zone!! " << zones[i].name << std::endl;
  // amp
  p += strlen(p);
  sprintf(p, "%f,", zones[i].amp);
  // origin
  p += strlen(p);
  sprintf(p, "%i,", zones[i].origin);
  // low key
  p += strlen(p);
  sprintf(p, "%i,", zones[i].low_key);
  // high key
  p += strlen(p);
  sprintf(p, "%i,", zones[i].high_key);
  // low vel
  p += strlen(p);
  sprintf(p, "%i,", zones[i].low_vel);
  // high vel
  p += strlen(p);
  sprintf(p, "%i,", zones[i].high_vel);
  // pitch
  p += strlen(p);
  sprintf(p, "%f,", zones[i].pitch_corr);
  // start
  p += strlen(p);
  sprintf(p, "%i,", zones[i].start);
  // left
  p += strlen(p);
  sprintf(p, "%i,", zones[i].left);
  // right
  p += strlen(p);
  sprintf(p, "%i,", zones[i].right);
  // loop mode
  p += strlen(p);
  sprintf(p, "%i,", zones[i].loop_mode);
  // crossfade
  p += strlen(p);
  sprintf(p, "%i,", zones[i].crossfade);
  // group
  p += strlen(p);
  sprintf(p, "%i,", zones[i].group);
  // off group
  p += strlen(p);
  sprintf(p, "%i,", zones[i].off_group);
  // attack
  p += strlen(p);
  sprintf(p, "%i,", zones[i].attack);
  // hold
  p += strlen(p);
  sprintf(p, "%i,", zones[i].hold);
  // decay
  p += strlen(p);
  sprintf(p, "%i,", zones[i].decay);
  // sustain
  p += strlen(p);
  sprintf(p, "%f,", zones[i].sustain);
  // release
  p += strlen(p);
  sprintf(p, "%i,", zones[i].release);
  // path
  p += strlen(p);
  sprintf(p, "%s\n", zones[i].path);
}

int process_callback(jack_nframes_t nframes, void* arg) {
  client_data* cli_data = static_cast<client_data*>(arg);
  sample_t* buffer1 = (sample_t*) jack_port_get_buffer(cli_data->output_port1, nframes);
  sample_t* buffer2 = (sample_t*) jack_port_get_buffer(cli_data->output_port2, nframes);
  memset(buffer1, 0, sizeof(sample_t) * nframes);
  memset(buffer2, 0, sizeof(sample_t) * nframes);

  cli_data->sampler->pre_process(nframes);

  // capture midi event
  void* midi_buf = jack_port_get_buffer(cli_data->input_port, nframes);

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
        if ((event.buffer[0] & 0x0f) == cli_data->channel) {
          // process note on
          if ((event.buffer[0] & 0xf0) == 0x90) {
            // yes, using mutex here violates the laws of real time ..BUT..
            // we don't expect a musician to tweak zones during an actual take!
            // this allows for demoing zone changes in thread safe way in *almost* real time
            // we can safely assume this mutex will be unlocked in a real take
            pthread_mutex_lock(&cli_data->zone_lock);
            cli_data->sampler->handle_note_on(event.buffer, nframes, n);
            pthread_mutex_unlock(&cli_data->zone_lock);
          }
          // process note off
          else if ((event.buffer[0] & 0xf0) == 0x80) {
            cli_data->sampler->handle_note_off(event.buffer);
          }
          // process sustain pedal
          else if ((event.buffer[0] & 0xf0) == 0xb0 && event.buffer[1] == 0x40) {
            cli_data->sampler->handle_sustain(event.buffer);
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
    cli_data->sampler->process_frame(n, 1.0, buffer1, buffer2);
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

  client_data* cli_data = new client_data;

  jack_set_process_callback(client, process_callback, cli_data);
  cli_data->input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  cli_data->output_port1 = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  cli_data->output_port2 = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  cli_data->zones.reserve(100);

  pthread_mutex_init(&cli_data->zone_lock, NULL);

  jack_nframes_t sample_rate = jack_get_sample_rate(client);
  // supposed to also implement jack_set_buffer_size_callback; for now assume rarely changes
  jack_nframes_t jack_buf_size = jack_get_buffer_size(client);
  cli_data->sampler = new JMSampler(cli_data->zones, sample_rate, jack_buf_size, jack_buf_size);

  cli_data->zone_number = 1;
  cli_data->channel = 0;

  if (jack_activate(client)) {
    jack_port_unregister(client, cli_data->input_port);
    jack_port_unregister(client, cli_data->output_port1);
    jack_port_unregister(client, cli_data->output_port2);
    jack_client_close(client);
    delete cli_data->sampler;
    pthread_mutex_destroy(&cli_data->zone_lock);
    delete cli_data;
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

  fprintf(fout, "update_vol:16\n");
  fflush(fout);

  while (fgets(buf, 256, fin) != NULL) {
    // kill newline char
    buf[strlen(buf) - 1] = '\0';
    std::cout << "UI: " << buf << std::endl;
    if (!strncmp(buf, "add_zone:", 9)) {
      char* p = strtok(buf + 9, ",");
      int index = atoi(p);
      p = strtok(NULL, ",");

      if (cli_data->waves.find(p) == cli_data->waves.end()) {
        cli_data->waves[p] = jm::parse_wave(p);
      }

      jm::wave& wav = cli_data->waves[p];
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
      sprintf(zone.name, "Zone %i", cli_data->zone_number++);
      strcpy(zone.path, p);

      char outstr[256];
      if (index < 0) {
        pthread_mutex_lock(&cli_data->zone_lock);
        cli_data->zones.push_back(zone);
        pthread_mutex_unlock(&cli_data->zone_lock);
        build_zone_str(outstr, cli_data->zones, cli_data->zones.size() - 1);
      }
      else {
        pthread_mutex_lock(&cli_data->zone_lock);
        cli_data->zones.insert(cli_data->zones.begin() + index, zone);
        pthread_mutex_unlock(&cli_data->zone_lock);
        build_zone_str(outstr, cli_data->zones, index);
      }

      fprintf(fout, outstr);
      fflush(fout);
    }
    else if (!strncmp(buf, "update_zone:", 12)) {
      char* p = strtok(buf + 12, ",");
      int index = atoi(p);
      p = strtok(NULL, ",");
      int key = atoi(p);

      p = strtok(NULL, ",");

      pthread_mutex_lock(&cli_data->zone_lock);
      switch (key) {
        case jm::ZONE_NAME:
          strcpy(cli_data->zones[index].name, p);
          break;
        case jm::ZONE_AMP:
          cli_data->zones[index].amp = atof(p);
          break;
        case jm::ZONE_ORIGIN:
          cli_data->zones[index].origin = atoi(p);
          break;
        case jm::ZONE_LOW_KEY:
          cli_data->zones[index].low_key = atoi(p);
          break;
        case jm::ZONE_HIGH_KEY:
          cli_data->zones[index].high_key = atoi(p);
          break;
        case jm::ZONE_LOW_VEL:
          cli_data->zones[index].low_vel = atoi(p);
          break;
        case jm::ZONE_HIGH_VEL:
          cli_data->zones[index].high_vel = atoi(p);
          break;
        case jm::ZONE_PITCH:
          cli_data->zones[index].pitch_corr = atof(p);
          break;
        case jm::ZONE_START:
          cli_data->zones[index].start = atoi(p);
          break;
        case jm::ZONE_LEFT:
          cli_data->zones[index].left = atoi(p);
          break;
        case jm::ZONE_RIGHT:
          cli_data->zones[index].right = atoi(p);
          break;
        case jm::ZONE_LOOP_MODE:
          cli_data->zones[index].loop_mode = (jm::loop_mode) atoi(p);
          break;
        case jm::ZONE_CROSSFADE:
          cli_data->zones[index].crossfade = atoi(p);
          break;
        case jm::ZONE_GROUP:
          cli_data->zones[index].group = atoi(p);
          break;
        case jm::ZONE_OFF_GROUP:
          cli_data->zones[index].off_group = atoi(p);
          break;
        case jm::ZONE_ATTACK:
          cli_data->zones[index].attack = atoi(p);
          break;
        case jm::ZONE_HOLD:
          cli_data->zones[index].hold = atoi(p);
          break;
        case jm::ZONE_DECAY:
          cli_data->zones[index].decay = atoi(p);
          break;
        case jm::ZONE_SUSTAIN:
          cli_data->zones[index].sustain = atof(p);
          break;
        case jm::ZONE_RELEASE:
          cli_data->zones[index].release = atoi(p);
          break;
      }
      pthread_mutex_unlock(&cli_data->zone_lock);
    }
  }

  fclose(fout);
  waitpid(pid, NULL, 0);

  jack_deactivate(client);
  jack_port_unregister(client, cli_data->input_port);
  jack_port_unregister(client, cli_data->output_port1);
  jack_port_unregister(client, cli_data->output_port2);
  jack_client_close(client);

  std::map<std::string, jm::wave>::iterator it;
  for (it = cli_data->waves.begin(); it != cli_data->waves.end(); ++it)
    jm::free_wave(it->second);

  delete cli_data->sampler;
  pthread_mutex_destroy(&cli_data->zone_lock);
  delete cli_data;

  return 0;
}
