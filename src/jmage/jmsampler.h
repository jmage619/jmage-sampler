#ifndef JM_JM_SAMPLER_H
#define JM_JM_SAMPLER_H

#include <jack/types.h>

#include "jmage/sampler.h"
#include "jmage/collections.h"
#include "jmage/components.h"

#define VOL_STEPS 17
// hardcode 1 zone until we come up with dynamic zone creation
#define NUM_ZONES 1
#define WAV_OFF_Q_SIZE 10
#define MSG_Q_SIZE 32

class JMSampler {
  private:
    // Jack stuff
    jack_client_t* client;
    jack_port_t* input_port;
    jack_port_t* output_port1;
    jack_port_t* output_port2;

    // for message passing
    JMQueue<jm_msg*> msg_q_in;
    JMQueue<jm_msg*> msg_q_out;

    // volume control; should amp really be a member?
    double amp[VOL_STEPS];
    int level;

    jm_key_zone jm_zones[NUM_ZONES];

    // state
    bool sustain_on;
    PlayheadList playheads;

    static void init_amp(JMSampler* jms);

  public:
    JMSampler();
    ~JMSampler();
    static int process_callback(jack_nframes_t nframes, void *arg);
};

#endif
