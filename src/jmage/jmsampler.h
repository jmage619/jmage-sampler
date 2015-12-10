#ifndef JM_JMSAMPLER_H
#define JM_JMSAMPLER_H

#include <vector>

#include <pthread.h>
#include <jack/types.h>

#include "jmage/sampler.h"
#include "jmage/collections.h"
#include "jmage/components.h"

#define VOL_STEPS 17
// hardcode 1 zone until we come up with dynamic zone creation
#define MAX_PLAYHEADS 10
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
    double amp[VOL_STEPS];
    int level;

    std::vector<jm_key_zone> zones;
    pthread_mutex_t zone_lock;

    // state
    bool sustain_on;
    PlayheadList playheads;

    jack_nframes_t jack_buf_size;
    sample_t* pitch_buf_arr;
    JMStack<sample_t*> pitch_buf_pool;

    static void init_amp(JMSampler* jms);

  public:
    JMSampler();
    ~JMSampler();
    void add_zone(const jm_key_zone& zone);
    const jm_key_zone& get_zone(int index);
    void update_zone(int index, const jm_key_zone& zone);
    void remove_zone(int index);
    size_t num_zones();
    void send_msg(const jm_msg& msg);
    bool receive_msg(jm_msg& msg);
    static int process_callback(jack_nframes_t nframes, void *arg);
};

#endif
