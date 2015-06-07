#ifndef JM_COMPONENTS_H
#define JM_COMPONENTS_H

#include <jack/jack.h>
#include "jmage/collections.h"

typedef jack_default_audio_sample_t sample_t;

class Playhead {
  public:
    int pitch;
    double amp;
    double speed;
    double position;
    int released;
    int note_off;
    int rel_time;
    sample_t* wave[2];
    jack_nframes_t wave_length;
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
    double pitch_corr;

    KeyZone();
    bool in(int pitch);
    void to_ph(Playhead& ph, int pitch, int velocity);
};

#endif
