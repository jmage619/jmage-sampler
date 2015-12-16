#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <samplerate.h>
#include <jack/types.h>

#include "jmage/collections.h"
#include "jmage/sampler.h"

#define NUM_PITCH_BUFS 2

class SoundGenerator {
  public:
    bool note_off;
    int pitch;
    virtual ~SoundGenerator(){}
    void init(int pitch){note_off = false; this->pitch = pitch;}
    virtual void pre_process(jack_nframes_t nframes){}
    virtual void inc() = 0;
    virtual void get_values(double values[]) = 0;
    virtual void set_release() = 0;
    virtual bool is_finished() = 0;
    virtual void release_resources() = 0;
};

class Playhead: public SoundGenerator {
  private:
    enum State {
      PLAYING,
      FINISHED
    } state;
    JMStack<Playhead*>* playhead_pool;
    bool loop_on;
    SRC_STATE* resampler;
    sample_t* pitch_bufs[NUM_PITCH_BUFS];
    jack_nframes_t pitch_buf_size;
    double speed;
    bool crossfading;
    jack_nframes_t cf_timer;
    sample_t* wave[2];
    jack_nframes_t wave_length;
    jack_nframes_t start;
    jack_nframes_t left;
    jack_nframes_t right;
    //double positions[2];
    jack_nframes_t in_offset;
    jack_nframes_t out_offset;
    bool last_iteration;
    jack_nframes_t position;
    //int first_pos;
    //int pos_size;
    jack_nframes_t crossfade;

  public:
    Playhead(JMStack<Playhead*>* playhead_pool, jack_nframes_t pitch_buf_size);
    ~Playhead();
    void init(const jm_key_zone& zone, int pitch);
    void pre_process(jack_nframes_t nframes);
    void inc();
    void get_values(double values[]);
    void set_release() {state = FINISHED;}
    bool is_finished(){return state == FINISHED;}
    void release_resources() {playhead_pool->push(this);}
};

class AmpEnvGenerator: public SoundGenerator {
  private:
    enum State {
      ATTACK,
      HOLD,
      DECAY,
      SUSTAIN,
      RELEASE,
      FINISHED
    } state;
    JMStack<AmpEnvGenerator*>* amp_gen_pool;
    SoundGenerator* sg;
    double amp;
    jack_nframes_t attack;
    jack_nframes_t hold;
    jack_nframes_t decay;
    double sustain;
    jack_nframes_t release;
    jack_nframes_t timer;
    double rel_amp;

    double get_amp();
  public:
    AmpEnvGenerator(JMStack<AmpEnvGenerator*>* amp_gen_pool): amp_gen_pool(amp_gen_pool) {}
    void init(SoundGenerator* sg, const jm_key_zone& zone, int pitch, int velocity);
    void pre_process(jack_nframes_t nframes) {sg->pre_process(nframes);}
    void inc();
    void get_values(double values[]);
    void set_release();
    bool is_finished(){return state == FINISHED;}
    void release_resources() {sg->release_resources(); amp_gen_pool->push(this);}
};

struct sg_list_el {
  SoundGenerator* sg;
  sg_list_el* next;
  sg_list_el* prev;
};

class SoundGenList {
  private:
    sg_list_el* head;
    sg_list_el* tail;
    size_t length;
    size_t m_size;
    JMStack<sg_list_el*> unused;
  public:
    SoundGenList(size_t length);
    ~SoundGenList();
    sg_list_el* get_head_ptr() {return head;}
    sg_list_el* get_tail_ptr() {return tail;}
    size_t size() {return m_size;}
    void add(SoundGenerator* sg);
    void remove(sg_list_el* sg_el);
    void remove_last();
};

#endif
