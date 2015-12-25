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

void AudioStream::init(const jm_key_zone& zone) {
  loop_on = zone.loop_on;
  crossfading = false;
  cf_timer = 0;
  wave = zone.wave;
  wave_length = zone.wave_length;
  num_channels = zone.num_channels;  
  cur_frame = zone.start;
  start = zone.start;
  left = zone.left;
  right = zone.right;
  crossfade = zone.crossfade;
}

int AudioStream::read(sample_t buf[], int nframes) {
  if (loop_on) {
    int out_offset = 0;
    int to_copy;
    int frames_left;

    if (crossfading) {
      //printf("goto jump to crossfade\n");
      goto crossfading;
    }

    while (1) {
      to_copy = (int) (right - crossfade / 2.0 - cur_frame);
      
      frames_left = nframes - out_offset / num_channels;
      if (to_copy > frames_left)
        to_copy = frames_left;

      memcpy(buf + out_offset, wave + num_channels * cur_frame, num_channels * to_copy * sizeof(sample_t));
      cur_frame += to_copy;
      out_offset += to_copy * num_channels;
      if (out_offset / num_channels >= nframes) {
        // if also hit crossfade, update state before leaving
        if (cur_frame >= right - crossfade / 2.0) {
          crossfading = true;
          cf_timer = 0;
          //printf("cf on exit\n");
        }
        return nframes;
      }

      crossfading = true;
      cf_timer = 0;
      //printf("cf on\n");
     
crossfading: 

      int fade_in_start = (int) (left - crossfade / 2.0);
      while (cf_timer < crossfade) {
        //printf("fade_in_pos1: %i\n", fade_in_pos);
        if (num_channels == 1) {
          buf[out_offset] = 0.0;
          if (cur_frame < wave_length)
            buf[out_offset] +=  (1.0 - cf_timer / (double) crossfade) * wave[cur_frame];
          if (fade_in_start + cf_timer >= 0) {
            buf[out_offset] +=  cf_timer / (double) crossfade * wave[fade_in_start + cf_timer];
          }
        }
        else {
          //printf("cf1: %f,%f\n",(1.0 - cf_timer / (double) crossfade),cf_timer / (double) crossfade);
          buf[out_offset] = 0.0;
          buf[out_offset + 1] = 0.0;
          if (cur_frame < wave_length) {
            buf[out_offset] +=  (1.0 - cf_timer / (double) crossfade) * wave[num_channels * cur_frame];
            buf[out_offset + 1] +=  (1.0 - cf_timer / (double) crossfade) * wave[num_channels * cur_frame + 1];
          }
          if (fade_in_start + cf_timer >= 0) {
            buf[out_offset] += cf_timer / (double) crossfade * wave[num_channels * fade_in_start + num_channels * cf_timer];
            buf[out_offset + 1] += cf_timer / (double) crossfade * wave[num_channels * fade_in_start + num_channels * cf_timer + 1];
          }
        }
        ++cur_frame;
        out_offset += num_channels;
        ++cf_timer;
        if (out_offset / num_channels >= nframes) {
          // if also hit crossfade end, update state before leaving
          if (cf_timer >= crossfade) {
            crossfading = false;
            //printf("cf off exit\n");
            cur_frame = (int) (left + crossfade / 2.0);
          }
          return nframes;
        }
      }
      crossfading = false;
      //printf("cf off\n");
      cur_frame = (int) (left + crossfade / 2.0);
    }
  }
  else {
    int frames_left = right - cur_frame;
    if (frames_left == 0)
      return 0;

    int to_copy = frames_left < nframes ? frames_left : nframes;
    
    memcpy(buf, wave + num_channels * cur_frame, num_channels * to_copy * sizeof(sample_t));
    cur_frame += to_copy;
    return to_copy;
  }
}

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

void Playhead::init(const jm_key_zone& zone, int pitch, jack_nframes_t start, jack_nframes_t right) {
  SoundGenerator::init(pitch);
  state = PLAYING;
  loop_on = zone.loop_on;
  crossfading = false;
  cf_timer = 0;
  wave_length = zone.wave_length;
  this->start = start;
  left = zone.left;
  this->right = right;
  //first_pos = 0;
  //pos_size = 1;
  crossfade = zone.crossfade;
  speed = pow(2, (pitch + zone.pitch_corr - zone.origin) / 12.);
  wave[0] = zone.wave;
  //wave[1] = zone.wave[1];
  //positions[0] = zone.start;
  in_offset = start;
  last_iteration = false;
  src_reset(resampler);
}

void Playhead::init(const jm_key_zone& zone, int pitch) {
  init(zone, pitch, zone.start, zone.right);
}

void Playhead::pre_process(jack_nframes_t nframes) {
  // needed?
  //memset(pitch_bufs[0], 0, sizeof(sample_t) * nframes);
  //memset(pitch_bufs[1], 0, sizeof(sample_t) * nframes);

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

void Looper::init(Playhead* ph1, Playhead* ph2, const jm_key_zone& zone, int pitch) {
  SoundGenerator::init(pitch);
  state = PLAYING;
  this->zone = zone;
  crossfading = false;
  cf_timer = 0;
  speed = pow(2, (pitch + zone.pitch_corr - zone.origin) / 12.);
  playheads[0] = ph1;
  playheads[1] = ph2;
  num_playheads = 1;
  first_ph = 0;
  left = zone.left / speed;
  right = zone.right / speed;
  position = (jack_nframes_t) (zone.start / speed);
  //printf("start pos: %u\n", position);
  crossfade = zone.crossfade / speed;
  playheads[first_ph]->init(zone, pitch, zone.start, zone.wave_length);
}

void Looper::pre_process(jack_nframes_t nframes) {
  for (int i = 0; i < num_playheads; ++i)
    playheads[(first_ph + i) % 2]->pre_process(nframes);
}

void Looper::inc() {
  for (int i = 0; i < num_playheads; ++i)
    playheads[(first_ph + i) % 2]->inc();

  ++position;

  if (!crossfading) {
    if (position >= right - crossfade / 2.0) {
      int next_ph = (first_ph + 1) % 2;
      playheads[next_ph]->init(zone, pitch, (jack_nframes_t) (speed * (position - right + left)), zone.wave_length);
      playheads[next_ph]->pre_process(playheads[next_ph]->get_pitch_buf_size());
      ++num_playheads;
      crossfading = true;
    }
  }
  // should not be else, in case in 1 step we go beyond entire crossfade
  if (crossfading) {
    if (position >= right + crossfade / 2.0 + CF_DELAY) {
        first_ph = (first_ph + 1) % 2;
        --num_playheads;
        crossfading = false;
        cf_timer = 0;
        position = (jack_nframes_t) (left + crossfade / 2.0);
    }
    else
      ++cf_timer;
  }
}

void Looper::get_values(double values[]) {
  if (crossfading) {
    double fade_out;
    double fade_in;
    if (crossfade <= 0.0) {
      fade_out = 1.0;
      fade_in = 1.0;
    }
    else {
      //printf("cf: %f; time: %d\n", crossfade, cf_timer);
      fade_out = -1.0 * (cf_timer - CF_DELAY) / crossfade + 1.0;
      if (fade_out > 1.0)
        fade_out = 1.0;
      else if (fade_out < 0.0)
        fade_out = 0.0;

      fade_in = (cf_timer - CF_DELAY) / crossfade;
      if (fade_in > 1.0)
        fade_in = 1.0;
      else if (fade_in < 0.0)
        fade_in = 0.0;
      //printf("cf_fimer: %d; cf: %f; fade in: %f; fade out: %f\n", cf_timer, crossfade, fade_in, fade_out);
    }
    double ph_values[2];
    playheads[first_ph]->get_values(ph_values);
    values[0] = fade_out * ph_values[0];
    values[1] = fade_out * ph_values[1];
    playheads[(first_ph + 1) % 2]->get_values(ph_values);
    values[0] += fade_in * ph_values[0];
    values[1] += fade_in * ph_values[1];
  }
  else
      playheads[first_ph]->get_values(values);
}

void Looper::release_resources() {
  playheads[0]->release_resources();
  playheads[1]->release_resources();
  looper_pool->push(this);
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
