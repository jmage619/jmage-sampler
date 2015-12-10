#include <cstdio>
#include <climits>
#include <cmath>

#include <jack/types.h>

#include "jmage/components.h"
#include "jmage/sampler.h"

#define MAX_VELOCITY 127
// boost for controllers that don't reach 127 easily
#define VELOCITY_BOOST 1.2

Playhead::Playhead():
  state(ATTACK),
  note_off(false),
  amp_timer(0),
  rel_amp(1.0),
  crossfading(false),
  cf_timer(0),
  first_pos(0),
  pos_size(1) {}

Playhead::Playhead(const jm_key_zone& zone, int pitch, int velocity):
    state(ATTACK),
    note_off(false),
    loop_on(zone.loop_on),
    pitch(pitch),
    attack(zone.attack),
    hold(zone.hold),
    decay(zone.decay),
    sustain(zone.sustain),
    release(zone.release),
    amp_timer(0),
    rel_amp(zone.sustain),
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
  switch (state) {
    case ATTACK:
    case HOLD:
    case DECAY:
    case RELEASE:
      ++amp_timer;
    default:
      break;
  }

  if (state == ATTACK && amp_timer >= attack) {
    //printf("ATTACK -> HOLD amp: %f\n", get_amp());
    state = HOLD;
    amp_timer = 0;
  }
  else if (state == HOLD && amp_timer >= hold) {
    //printf("HOLD -> DECAY amp: %f\n", get_amp());
    state = DECAY;
    amp_timer = 0;
  }
  else if (state == DECAY && amp_timer >= decay) {
    //printf("DECAY -> SUSTAIN amp: %f\n", get_amp());
    if (sustain == 0.0) {
      //printf("sus 0 FINISHED\n");
      state = FINISHED;
      return;
    }
    state = SUSTAIN;
    amp_timer = 0;
  }
  else if (state == RELEASE && amp_timer >= release) {
    //printf("rel FINISHED\n");
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
  switch (state) {
    case ATTACK:
      // envelope from 0.0 to 1.0
      if (attack != 0)
        return amp * (float) amp_timer / attack;
    case DECAY:
      // envelope from 1.0 to sustain
      if (decay != 0)
        return amp * (-(1.0 - sustain) * amp_timer / decay + 1.0);
    case SUSTAIN:
      return amp * sustain;
    case RELEASE:
      // from rel_amp to 0.0
      //return amp * (- rel_amp * amp_timer / release + rel_amp);
      if (release != 0)
        return - rel_amp * amp_timer / release + rel_amp;
      else
        return rel_amp;
    default:
      break;
  }
  return amp;
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

void Playhead::set_release() {
  rel_amp = get_amp();
  amp_timer = 0;
  state = RELEASE;
}

PlayheadList::PlayheadList(size_t length): 
  head(NULL), tail(NULL), length(length), m_size(0), unused(length) {
  arr = new ph_list_el[length];

  for (size_t i = 0; i < length; i++) {
    ph_list_el* pel = arr + i;
    unused.push(pel);
  }
}

PlayheadList::~PlayheadList() {
  delete [] arr;
}

void PlayheadList::add(const Playhead& ph) {
  ph_list_el* pel;
  unused.pop(pel);
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
  unused.push(pel);

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

