#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <samplerate.h>

#include "zone.h"
#include "collections.h"

// arbitrarily sized to max 16 bit int just because 16 bit is awesome
#define PH_BUF_SIZE 32768

class AudioStream {
  private:
    bool loop_on;
    bool crossfading;
    int cf_timer;
    // assume interleaved stereo
    float* wave;
    // offsets are int because even 32-bit signed int indexes will cover 2 hours of
    // stereo interleaved audio sampled at 192khz
    // i doubt normal sampler usage would require more than minutes of audio
    int wave_length; // length in frames
    int num_channels; // only implemented to handle 1 or 2 channels
    // offsets in frames
    int cur_frame;
    int start;
    int left;
    int right;
    int crossfade;

  public:
    void init(const jm::zone& zone);
    int read(float* buf, int nframes);
};

class SoundGenerator {
  public:
    bool note_off;
    bool one_shot;
    int pitch;
    int off_group;
    virtual ~SoundGenerator(){}
    void init(const jm::zone& zone, int pitch) {
      note_off = false;
      one_shot = (zone.mode == jm::LOOP_ONE_SHOT) ? true : false;
      off_group = zone.off_group;
      this->pitch = pitch;
    }
    virtual void pre_process(size_t /*nframes*/){}
    virtual void inc() = 0;
    virtual void get_values(float* values) = 0;
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
    JMStack<Playhead*>& playhead_pool;
    AudioStream as;
    SRC_STATE* resampler;
    float in_buf[PH_BUF_SIZE];
    float* pitch_buf;
    double speed;
    int num_channels; // only implemented to handle 1 or 2 channels
    int in_offset;
    int num_read;
    size_t out_offset;
    bool last_iteration;
    size_t cur_frame;

  public:
    Playhead(JMStack<Playhead*>& playhead_pool, size_t pitch_buf_size);
    ~Playhead();
    void init(const jm::zone& zone, int pitch);
    void pre_process(size_t nframes);
    void inc();
    void get_values(float* values);
    void set_release() {state = FINISHED;}
    bool is_finished(){return state == FINISHED;}
    void release_resources() {playhead_pool.push(this);}
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
    JMStack<AmpEnvGenerator*>& amp_gen_pool;
    SoundGenerator* sg;
    float amp;
    int attack;
    int hold;
    int decay;
    float sustain;
    int release;
    int timer;
    float env_rel_val;

    float get_env_val();
  public:
    AmpEnvGenerator(JMStack<AmpEnvGenerator*>& amp_gen_pool): amp_gen_pool(amp_gen_pool) {}
    void init(SoundGenerator* sg, const jm::zone& zone, int pitch, int velocity);
    void pre_process(size_t nframes) {sg->pre_process(nframes);}
    void inc();
    void get_values(float* values);
    void set_release();
    bool is_finished(){return state == FINISHED;}
    void release_resources() {sg->release_resources(); amp_gen_pool.push(this);}
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
