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

    //JMZoneList* zones;
    std::vector<jm_key_zone> zones;

    // state
    SoundGenList sound_gens;

    jack_nframes_t jack_buf_size;
    JMStack<Playhead*> playhead_pool;
    JMStack<AmpEnvGenerator*> amp_gen_pool;

    static void init_amp(JMSampler* jms);

  public:
    // volume control; should amp really be a member?
    float amp[VOL_STEPS];

    int level;
    int channel;
    bool sustain_on;
    //JMSampler(JMZoneList* zones);
    JMSampler();
    ~JMSampler();
    Playhead* new_playhead(){Playhead* ph;playhead_pool.pop(ph);return ph;}
    AmpEnvGenerator* new_amp_gen(){AmpEnvGenerator* ag;amp_gen_pool.pop(ag);return ag;}
    sg_list_el* sound_gens_head(){return sound_gens.get_head_ptr();}
    sg_list_el* sound_gens_tail(){return sound_gens.get_tail_ptr();}
    size_t sound_gens_size(){return sound_gens.size();}
    void sound_gens_remove(sg_list_el* sg_el){sound_gens.remove(sg_el);}
    void sound_gens_add(SoundGenerator* sg){sound_gens.add(sg);}
    void sound_gens_remove_last(){sound_gens.remove_last();}
    std::vector<jm_key_zone>::iterator zones_begin(){return zones.begin();}
    std::vector<jm_key_zone>::iterator zones_end(){return zones.end();}
    std::vector<jm_key_zone>::size_type zones_size(){return zones.size();}
    void zones_add(jm_key_zone zone){zones.push_back(zone);}
    void send_msg(const jm_msg& msg);
    bool receive_msg(jm_msg& msg);
    static int process_callback(jack_nframes_t nframes, void *arg);
};

#endif
