#include <limits.h>
#include <math.h>

#include <jack/types.h>

#include "jmage/components.h"
#include "jmage/sampler.h"

#define MAX_VELOCITY 127
// boost for controllers that don't reach 127 easily
#define VELOCITY_BOOST 1.2

Playhead::Playhead():
  state(PLAYING),
  rel_timer(0),
  crossfading(false),
  cf_timer(0),
  first_pos(0),
  pos_size(1) {}

Playhead::Playhead(jm_key_zone& zone, int pitch, int velocity):
    state(PLAYING),
    loop_on(zone.loop_on),
    pitch(pitch),
    rel_time(zone.rel_time),
    rel_timer(0),
    crossfading(false),
    cf_timer(0),
    wave_length(zone.wave_length),
    start(zone.start),
    left(zone.left),
    right(zone.right),
    first_pos(0),
    pos_size(1),
    crossfade(zone.crossfade) {
  double calc_amp = zone.amp * VELOCITY_BOOST * velocity / MAX_VELOCITY;
  amp = calc_amp > 1.0 ? 1.0 : calc_amp;
  speed = pow(2, (pitch + zone.pitch_corr - zone.origin) / 12.);
  wave[0] = zone.wave[0];
  wave[1] = zone.wave[1];
  positions[0] = zone.start;
}

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

