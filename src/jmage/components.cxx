#include <limits.h>
#include <math.h>
#include "jmage/components.h"

#define MAX_VELOCITY 127
// boost for controllers that don't reach 127 easily
#define VELOCITY_BOOST 1.2

Playhead::Playhead(): state(PLAYING), rel_amp(1.0), position(0) {}

void Playhead::inc() {
  if (state == RELEASED) {
    rel_amp = - (rel_timer / rel_time) + 1.0;
    rel_timer++;
  }
  position += speed;

  if ((jack_nframes_t) position >= wave_length || rel_timer >= rel_time)
    state = FINISHED;
}

double Playhead::get_amp() {
  return amp * rel_amp;
}

PlayheadList::PlayheadList(size_t length): 
  head(NULL), tail(NULL), length(length), m_size(0), unused(length) {
  arr = new ph_list_el[length];

  for (size_t i = 0; i < length; i++) {
    ph_list_el* pel = arr + i;
    unused.add(pel);
  }
}

PlayheadList::~PlayheadList() {
  delete [] arr;
  std::cout << " PlayheadList arr deleted" << std::endl;
}

void PlayheadList::add(Playhead ph) {
  ph_list_el* pel;
  unused.remove(pel);
  pel->ph = ph;

  pel->next = head;
  pel->prev = NULL;
  if (pel->next == NULL)
    tail = pel;
  else
    pel->next->prev = pel;

  head = pel;
  m_size++;
}

void PlayheadList::remove(ph_list_el* pel) {
  unused.add(pel);

  if (pel == head)
    head = head->next;
  else
    pel->prev->next = pel->next;

  if (pel == tail)
    tail = tail->prev;
  else
    pel->next->prev = pel->prev;

  m_size--;
}

void PlayheadList::remove_last() {
  remove(tail);
}

KeyZone::KeyZone():
  lower_bound(INT_MIN),
  upper_bound(INT_MAX),
  origin(48),
  amp(1.0),
  rel_time(0.0) {}

bool KeyZone::contains(int pitch) {
  if (pitch >= lower_bound && pitch <= upper_bound)
    return true;
  return false;
}

void KeyZone::to_ph(Playhead& ph, int pitch, int velocity) {
  ph.pitch = pitch;
  double calc_amp = amp * VELOCITY_BOOST * velocity / MAX_VELOCITY;
  ph.amp = calc_amp > 1.0 ? 1.0 : calc_amp;
  ph.speed = pow(2, (pitch + pitch_corr- origin) / 12.);
  ph.wave[0] = wave[0];
  ph.wave[1] = wave[1];
  ph.wave_length = wave_length;
  ph.rel_time = rel_time;
  ph.rel_timer = 0;
}
