#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <samplerate.h>
#include <jack/types.h>

#include "jmage/collections.h"
#include "jmage/sampler.h"

#define NUM_PITCH_BUFS 2
// delay crossfade by about 16 ms to minimize pops (assumed 44100)
// purely determined by trial and error
#define CF_DELAY 706.0
// arbitrarily sized to max 16 bit int just because 16 bit is awesome
#define PH_BUF_SIZE 32768

class AudioStream {
  private:
    bool loop_on;
    bool crossfading;
    int cf_timer;
    // assume interleaved stereo
    sample_t* wave;
    // offsets are int because even 32-bit signed int indexes will cover 2 hours of
    // stereo interleaved audio sampled at 192khz
    // i doubt normal sampler usage would require more than minutes of audio
    int wave_length; // length in frames
    int num_channels;
    // offsets in frames
    int cur_frame;
    int start;
    int left;
    int right;
    int crossfade;

  public:
    void init(const jm_key_zone& zone);
    int read(sample_t buf[], int nframes);
};

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
    //bool loop_on;
    AudioStream as;
    SRC_STATE* resampler;
    sample_t in_buf[PH_BUF_SIZE];
    sample_t* pitch_buf;
    jack_nframes_t pitch_buf_size;
    double speed;
    //bool crossfading;
    //jack_nframes_t cf_timer;
    //sample_t* wave[2];
    //jack_nframes_t wave_length;
    int num_channels;
    //jack_nframes_t start;
    //jack_nframes_t left;
    //jack_nframes_t right;
    //double positions[2];
    int in_offset;
    int num_read;
    jack_nframes_t out_offset;
    bool last_iteration;
    //jack_nframes_t position;
    jack_nframes_t cur_frame;
    //int first_pos;
    //int pos_size;
    //jack_nframes_t crossfade;

  public:
    Playhead(JMStack<Playhead*>* playhead_pool, jack_nframes_t pitch_buf_size);
    ~Playhead();
    void init(const jm_key_zone& zone, int pitch, jack_nframes_t start, jack_nframes_t right);
    void init(const jm_key_zone& zone, int pitch);
    void pre_process(jack_nframes_t nframes);
    void inc();
    void get_values(double values[]);
    void set_release() {state = FINISHED;}
    bool is_finished(){return state == FINISHED;}
    void release_resources() {playhead_pool->push(this);}
    jack_nframes_t get_pitch_buf_size() {return pitch_buf_size;}
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

class Looper: public SoundGenerator {
  private:
    enum State {
      PLAYING,
      FINISHED
    } state;
    JMStack<Looper*>* looper_pool;
    jm_key_zone zone;
    bool crossfading;
    jack_nframes_t cf_timer;
    double speed;
    Playhead* playheads[2];
    int num_playheads;
    int first_ph;
    double left;
    double right;
    jack_nframes_t position;
    double crossfade;
  public:
    Looper(JMStack<Looper*>* looper_pool): looper_pool(looper_pool) {}
    void init(Playhead* ph1, Playhead* ph2, const jm_key_zone& zone, int pitch);
    void pre_process(jack_nframes_t nframes);
    void inc();
    void get_values(double values[]);
    void set_release() {state = FINISHED;}
    bool is_finished(){return state == FINISHED;}
    void release_resources();
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
    void remove_last() {remove(tail);}
};

#endif
