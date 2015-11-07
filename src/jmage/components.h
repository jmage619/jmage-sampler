#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <jack/types.h>

#include "jmage/collections.h"
#include "jmage/sampler.h"

class Playhead {
  public:
    enum State {
      PLAYING,
      RELEASED,
      NOTE_OFF,
      FINISHED
    };
    State state;
    bool loop_on;
    int pitch;
    double amp;
    double speed;
    double rel_time;
    jack_nframes_t rel_timer;
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
    JMQueue<ph_list_el*> unused;
  public:
    PlayheadList(size_t length);
    ~PlayheadList();
    ph_list_el* get_head_ptr() {return head;}
    size_t size() {return m_size;}
    void add(const Playhead& ph);
    void remove(ph_list_el* pel);
    void remove_last();
};

#endif
