#include <limits.h>
#include <math.h>
#include "jmage/components.h"

#define MAX_VELOCITY 127
// boost for controllers that don't reach 127 easily
#define VELOCITY_BOOST 1.2

Playhead::Playhead():
  state(PLAYING),
  rel_timer(0),
  crossfading(false),
  cf_timer(0),
  first_pos(0) {}

void Playhead::inc() {
  if (state == RELEASED)
    rel_timer++;

  if (state == RELEASED && rel_timer >= rel_time) {
    state = FINISHED;
    return;
  }

  for (int i = 0; i < pos_size; i++) {
    positions[(first_pos + i) % 2] += speed;
  }

  if (loop_on) {
    if (!crossfading) {
      if (positions[first_pos] >= right - crossfade / 2.0) {
        positions[(first_pos + 1) % 2] = positions[first_pos] - (right - left);
        pos_size++;
        crossfading = true;
      }
    }
    else {
      cf_timer++;
      if (positions[first_pos] >= right + crossfade / 2.0) {
          first_pos = (first_pos + 1) % 2;
          pos_size--;
          crossfading = false;
          cf_timer = 0;
      }
    }
  }
  else if ((jack_nframes_t) positions[first_pos] >= right)
    state = FINISHED;
}

double Playhead::get_amp() {
  return state == RELEASED ? amp * (-(rel_timer / rel_time) + 1.0) : amp;
}

void Playhead::get_values(double values[]) {
  if (crossfading) {
    values[0] = 0;
    values[1] = 0;
    double fade_out = -1.0 * cf_timer / (crossfade / speed) + 1.0;
    double fade_in = 1.0 * cf_timer / (crossfade / speed);
    if (positions[first_pos] < wave_length) {
      values[0] += get_amp() * fade_out * wave[0][(jack_nframes_t) positions[first_pos]];
      values[1] += get_amp() * fade_out * wave[1][(jack_nframes_t) positions[first_pos]];
    }
    if (positions[(first_pos + 1) % 2] >= 0) {
      values[0] += get_amp() * fade_in * wave[0][(jack_nframes_t) positions[(first_pos + 1) % 2]];
      values[1] += get_amp() * fade_in * wave[1][(jack_nframes_t) positions[(first_pos + 1) % 2]];
    }
  }
  else {
    values[0] = get_amp() * wave[0][(jack_nframes_t) positions[first_pos]];
    values[1] = get_amp() * wave[1][(jack_nframes_t) positions[first_pos]];
  }
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
  start(0),
  left(0),
  lower_bound(INT_MIN),
  upper_bound(INT_MAX),
  origin(48),
  amp(1.0),
  rel_time(0.0),
  pitch_corr(0.0),
  loop_on(false),
  crossfade(0) {}

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
  ph.start = start;
  ph.positions[0] = start;
  ph.pos_size = 1;
  ph.left = left;
  ph.right = right;
  ph.rel_time = rel_time;
  ph.loop_on = loop_on;
  ph.crossfade = crossfade;
}
