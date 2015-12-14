#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <samplerate.h>
#include <jack/types.h>

#include "jmage/collections.h"
#include "jmage/sampler.h"

#define NUM_PITCH_BUFS 2

class Playhead {
  public:
    enum State {
      ATTACK,
      HOLD,
      DECAY,
      SUSTAIN,
      RELEASE,
      FINISHED
    };
    JMStack<Playhead*>* playhead_pool;
    State state;
    bool note_off;
    bool loop_on;
    SRC_STATE* resampler;
    int pitch;
    sample_t* pitch_bufs[NUM_PITCH_BUFS];
    jack_nframes_t pitch_buf_size;
    double amp;
    double speed;
    jack_nframes_t attack;
    jack_nframes_t hold;
    jack_nframes_t decay;
    double sustain;
    jack_nframes_t release;
    jack_nframes_t amp_timer;
    double rel_amp;
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

    Playhead(JMStack<Playhead*>* playhead_pool, jack_nframes_t pitch_buf_size);
    ~Playhead();
    void init(const jm_key_zone& zone, int pitch, int velocity);
    void pre_process(jack_nframes_t nframes);
    void inc();
    double get_amp();
    void get_values(double values[]);
    void set_release();
    void release_resources();
};

struct ph_list_el {
  Playhead* ph;
  ph_list_el* next;
  ph_list_el* prev;
};

class PlayheadList {
  private:
    ph_list_el* head;
    ph_list_el* tail;
    size_t length;
    size_t m_size;
    JMStack<ph_list_el*> unused;
  public:
    PlayheadList(size_t length);
    ~PlayheadList();
    ph_list_el* get_head_ptr() {return head;}
    ph_list_el* get_tail_ptr() {return tail;}
    size_t size() {return m_size;}
    void add(Playhead* ph);
    void remove(ph_list_el* pel);
    void remove_last();
};

#endif
