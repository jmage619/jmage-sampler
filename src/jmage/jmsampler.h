#ifndef JM_JMSAMPLER_H
#define JM_JMSAMPLER_H

#include <vector>
#include <pthread.h>
#include <jack/types.h>

#include "jmage/sampler.h"
//#include "jmage/jmzonelist.h"
#include "jmage/collections.h"
#include "jmage/components.h"

#define VOL_STEPS 17
//#define MAX_PLAYHEADS 10
#define POLYPHONY 10
#define MSG_Q_SIZE 32

class JMSampler {
  private:
    // Jack stuff
    jack_client_t* client;
    jack_port_t* input_port;
    jack_port_t* output_port1;
    jack_port_t* output_port2;

    // for message passing
    JMQueue<jm_msg> msg_q_in;
    JMQueue<jm_msg> msg_q_out;

    // volume control; should amp really be a member?
    float amp[VOL_STEPS];
    int level;
    int channel;

    //JMZoneList* zones;
    std::vector<jm_key_zone> zones;

    // state
    bool sustain_on;
    SoundGenList sound_gens;

    jack_nframes_t jack_buf_size;
    JMStack<Playhead*> playhead_pool;
    JMStack<AmpEnvGenerator*> amp_gen_pool;

    static void init_amp(JMSampler* jms);

  public:
    //JMSampler(JMZoneList* zones);
    JMSampler();
    ~JMSampler();
    void send_msg(const jm_msg& msg);
    bool receive_msg(jm_msg& msg);
    static int process_callback(jack_nframes_t nframes, void *arg);
};

#endif
