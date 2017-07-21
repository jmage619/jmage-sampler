#ifndef JACKSAMPLER_H
#define JACKSAMPLER_H

#include <vector>
#include <map>
#include <pthread.h>

#include <jack/types.h>

#include <lib/zone.h>
#include <lib/wave.h>
#include <lib/collections.h>
#include <lib/sfzparser.h>
#include <lib/jmsampler.h>

enum msg_type {
MT_VOLUME,
MT_CHANNEL
};

struct jm_msg {
  msg_type type;
  union {
    int i;
  } data;
};

struct jack_sampler {
  std::map<std::string, jm::wave> waves;
  std::vector<jm::zone> zones;
  pthread_mutex_t zone_lock;
  JMQueue<jm_msg>* msg_q;
  jack_port_t* input_port;
  jack_port_t* output_port1;
  jack_port_t* output_port2;
  JMSampler* sampler;
  int sample_rate;
  int zone_number;
  int volume;
  int channel;
};

void build_zone_str(char* outstr, const std::vector<jm::zone>& zones, int i);
void add_zone_from_region(jack_sampler* sampler, const std::map<std::string, SFZValue>& region);
void save_patch(jack_sampler* sampler, const char* path);

#endif
