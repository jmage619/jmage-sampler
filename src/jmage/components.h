#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <jack/jack.h>
#include "jmage/collections.h"

typedef jack_default_audio_sample_t sample_t;

class Playhead {
  public:
    enum State {
      PLAYING,
      RELEASED,
      NOTE_OFF,
      FINISHED
    };
    State state;
    int pitch;
    double amp;
    double rel_amp;
    double speed;
    double position;
    double rel_time;
    jack_nframes_t rel_timer;
    sample_t* wave[2];
    jack_nframes_t wave_length;

    Playhead();
    void inc();
    double get_amp();
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
    void add(Playhead ph);
    void remove(ph_list_el* pel);
    void remove_last();
};

class KeyZone {
  public:
    sample_t* wave[2];
    jack_nframes_t wave_length;
    int lower_bound;
    int upper_bound;
    int origin;
    double amp;
    double rel_time;
    double pitch_corr;

    KeyZone();
    bool contains(int pitch);
    void to_ph(Playhead& ph, int pitch, int velocity);
};

#endif
