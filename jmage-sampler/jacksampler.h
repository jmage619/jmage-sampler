#ifndef JACKSAMPLER_H
#define JACKSAMPLER_H

#include <cstdio>
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
    float f;
  } data;
};

class JackSampler: public JMSampler {
  public:
    jack_port_t* input_port;
    jack_port_t* output_port1;
    jack_port_t* output_port2;
    float _volume;
    float _channel;
    FILE* fout;
    JMQueue<jm_msg> msg_q;

    JackSampler(int sample_rate, size_t in_nframes, size_t out_nframes);
    void send_add_zone(int index);
    void send_update_wave(int index);
};

#endif
