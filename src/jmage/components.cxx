#include <cstdio>
#include <climits>
#include <cmath>
#include <cstring>

#include <samplerate.h>
#include <jack/types.h>

#include "jmage/components.h"
#include "jmage/sampler.h"

#define MAX_VELOCITY 127
// boost for controllers that don't reach 127 easily
#define VELOCITY_BOOST 1.2

Playhead::Playhead(JMStack<Playhead*>* playhead_pool, jack_nframes_t pitch_buf_size):
    playhead_pool(playhead_pool), 
    pitch_buf_size(pitch_buf_size),
    crossfading(false),
    cf_timer(0)
    //first_pos(0),
    //pos_size(1)
    {
  pitch_bufs[0] = new sample_t[pitch_buf_size];
  pitch_bufs[1] = new sample_t[pitch_buf_size];
  int error;
  resampler = src_new(SRC_SINC_FASTEST, 1, &error);
}

Playhead::~Playhead() {
  src_delete(resampler);
  delete [] pitch_bufs[0];
  delete [] pitch_bufs[1];
}

void Playhead::init(const jm_key_zone& zone, int pitch) {
  SoundGenerator::init(pitch);
  state = PLAYING;
  loop_on = zone.loop_on;
  crossfading = false;
  cf_timer = 0;
  wave_length = zone.wave_length;
  start = zone.start;
  left = zone.left;
  right = zone.right;
  //first_pos = 0;
  //pos_size = 1;
  crossfade = zone.crossfade;
  speed = pow(2, (pitch + zone.pitch_corr - zone.origin) / 12.);
  wave[0] = zone.wave[0];
  wave[1] = zone.wave[1];
  //positions[0] = zone.start;
  in_offset = zone.start;
  last_iteration = false;
  src_reset(resampler);
}

void Playhead::pre_process(jack_nframes_t nframes) {
  out_offset = 0;

  SRC_DATA data;
  data.src_ratio = 1 / speed;
  data.end_of_input = 0;

  while (1) {
    data.data_in = wave[0] + in_offset;
    data.input_frames = wave_length - in_offset;
    data.data_out = pitch_bufs[0] + out_offset;
    data.output_frames = nframes - out_offset;
    src_process(resampler, &data);
    // for now just duping L channel to make mono
    memcpy(pitch_bufs[1] + out_offset, pitch_bufs[0] + out_offset, data.output_frames_gen * sizeof(sample_t));
    out_offset += data.output_frames_gen;
    in_offset += data.input_frames_used;
    if (in_offset >= right) {
      last_iteration = true;
      break;
    }
    if (out_offset >= nframes)
      break;
  }
  position = 0;
}

void Playhead::inc() {
  ++position;

  /*if (loop_on) {
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
  */
  //if ((jack_nframes_t) positions[first_pos] >= right)
  if (last_iteration && position >= out_offset)
    state = FINISHED;
}

void Playhead::get_values(double values[]) {
  /*if (crossfading) {
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
  */
  values[0] = pitch_bufs[0][position];
  values[1] = pitch_bufs[1][position];
}

void AmpEnvGenerator::init(SoundGenerator* sg, const jm_key_zone& zone, int pitch, int velocity) {
  SoundGenerator::init(pitch);
  this->sg = sg;
  state = ATTACK;
  attack = zone.attack;
  hold = zone.hold;
  decay = zone.decay;
  sustain = zone.sustain;
  release = zone.release;
  timer = 0;
  rel_amp = zone.sustain;
  double calc_amp = zone.amp * VELOCITY_BOOST * velocity / MAX_VELOCITY;
  amp = calc_amp > 1.0 ? 1.0 : calc_amp;
}

void AmpEnvGenerator::inc() {
  sg->inc();
  if (sg->is_finished()) {
    state = FINISHED;
    return;
  }
  switch (state) {
    case ATTACK:
    case HOLD:
    case DECAY:
    case RELEASE:
      ++timer;
    default:
      break;
  }

  if (state == ATTACK && timer >= attack) {
    state = HOLD;
    timer = 0;
  }
  else if (state == HOLD && timer >= hold) {
    state = DECAY;
    timer = 0;
  }
  else if (state == DECAY && timer >= decay) {
    if (sustain == 0.0) {
      state = FINISHED;
      return;
    }
    state = SUSTAIN;
    timer = 0;
  }
  else if (state == RELEASE && timer >= release) {
    state = FINISHED;
    return;
  }
}

double AmpEnvGenerator::get_amp() {
  switch (state) {
    case ATTACK:
      // envelope from 0.0 to 1.0
      if (attack != 0)
        return amp * (float) timer / attack;
      break;
    case DECAY:
      // envelope from 1.0 to sustain
      if (decay != 0)
        return amp * (-(1.0 - sustain) * timer / decay + 1.0);
      break;
    case SUSTAIN:
      return amp * sustain;
    case RELEASE:
      // from rel_amp to 0.0
      if (release != 0)
        return - rel_amp * timer / release + rel_amp;
      else
        return rel_amp;
    default:
      break;
  }
  return amp;
}

void AmpEnvGenerator::get_values(double values[]) {
  double cur_amp = get_amp();
  sg->get_values(values);
  values[0] *= cur_amp;
  values[1] *= cur_amp;
}

void AmpEnvGenerator::set_release() {
  rel_amp = get_amp();
  timer = 0;
  state = RELEASE;
}

SoundGenList::SoundGenList(size_t length): 
  head(NULL), tail(NULL), length(length), m_size(0), unused(length) {

  for (size_t i = 0; i < length; i++) {
    sg_list_el* sg_el = new sg_list_el;
    unused.push(sg_el);
  }
}

SoundGenList::~SoundGenList() {
  while (m_size > 0)
    remove_last();

  sg_list_el* sg_el;
  while (unused.pop(sg_el)) {
    delete sg_el;
  }
}

void SoundGenList::add(SoundGenerator* sg) {
  sg_list_el* sg_el;
  unused.pop(sg_el);
  sg_el->sg = sg;

  sg_el->next = head;
  sg_el->prev = NULL;
  if (sg_el->next == NULL)
    tail = sg_el;
  else
    sg_el->next->prev = sg_el;

  head = sg_el;
  m_size++;
}

void SoundGenList::remove(sg_list_el* sg_el) {
  unused.push(sg_el);

  if (sg_el == head)
    head = head->next;
  else
    sg_el->prev->next = sg_el->next;

  if (sg_el == tail)
    tail = tail->prev;
  else
    sg_el->next->prev = sg_el->prev;

  m_size--;
}

void SoundGenList::remove_last() {
  remove(tail);
}

