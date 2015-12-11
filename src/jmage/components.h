#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <jack/types.h>

#include "jmage/collections.h"
#include "jmage/sampler.h"

#define NUM_PITCH_BUFS 4

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
    State state;
    bool note_off;
    bool loop_on;
    int pitch;
    sample_t* pitch_bufs[NUM_PITCH_BUFS];
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
    double positions[2];
    int first_pos;
    int pos_size;
    jack_nframes_t crossfade;

    Playhead();
    Playhead(const jm_key_zone& zone, int pitch, int velocity);
    void inc();
    double get_amp();
    void get_values(double values[]);
    void set_release();
    void set_pitch_bufs(JMStack<sample_t*>& pitch_buf_pool);
    void release_pitch_bufs(JMStack<sample_t*>& pitch_buf_pool);
};

struct ph_list_el {
  Playhead ph;
  ph_list_el* next;
  ph_list_el* prev;
};

class PlayheadList {
  private:
    ph_list_el* head;
    ph_list_el* tail;
    ph_list_el* arr;
    size_t length;
    size_t m_size;
    JMStack<ph_list_el*> unused;
  public:
    PlayheadList(size_t length);
    ~PlayheadList();
    ph_list_el* get_head_ptr() {return head;}
    ph_list_el* get_tail_ptr() {return tail;}
    size_t size() {return m_size;}
    void add(const Playhead& ph);
    void remove(ph_list_el* pel);
    void remove_last();
};

#endif
