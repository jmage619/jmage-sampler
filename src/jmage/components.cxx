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
    playhead_pool(playhead_pool) {
  // buf size * 2 to make room for stereo
  pitch_buf = new sample_t[pitch_buf_size * 2];
  int error;
  // have to always make it stereo since they are allocated in advance
  resampler = src_new(SRC_SINC_FASTEST, 2, &error);
}

Playhead::~Playhead() {
  src_delete(resampler);
  delete [] pitch_buf;
}

void Playhead::init(const jm_key_zone& zone, int pitch) {
  SoundGenerator::init(pitch);
  as.init(zone);
  num_channels = zone.num_channels;
  state = PLAYING;
  speed = pow(2, (pitch + zone.pitch_corr - zone.origin) / 12.);
  in_offset = 0;

  num_read = as.read(in_buf, PH_BUF_SIZE / 2);
  // if mono, just duplicate and interleave values
  // note above read is still valid, for mono we just read half the buffer
  if (num_channels == 1) {
    //printf("init num read: %i\n", num_read);
    for (int i = 0; i < num_read; ++i) {

      //printf("i: %i; src: %i; dest: %i\n", i, num_read - 1 - i, 2 * (num_read - 1 - i));
      in_buf[2 * (num_read - 1 - i)] = in_buf[num_read - 1 - i];
      in_buf[2 * (num_read - 1 - i) + 1] = in_buf[num_read - 1 - i];
    }
  }

  last_iteration = false;
  src_reset(resampler);
}

void Playhead::pre_process(jack_nframes_t nframes) {
  memset(pitch_buf, 0,  2 * nframes * sizeof(sample_t));

  out_offset = 0;

  SRC_DATA data;
  data.src_ratio = 1 / speed;
  data.end_of_input = 0;

  while (out_offset < nframes) {
    data.data_in = in_buf + 2 * in_offset;
    data.input_frames = num_read - in_offset;
    data.data_out = pitch_buf + 2 * out_offset;
    data.output_frames = nframes - out_offset;
    src_process(resampler, &data);

    out_offset += data.output_frames_gen;
    in_offset += data.input_frames_used;

    if (in_offset >= num_read) {
      num_read = as.read(in_buf, PH_BUF_SIZE / 2);

      if (num_read == 0) {
        last_iteration = true;
        break;
      }
      // if mono, just duplicate and interleave values
      // note above read is still valid, for mono we just read half the buffer
      if (num_channels == 1) {
        for (int i = 0; i < num_read; ++i) {
          in_buf[2 * (num_read - 1 - i)] = in_buf[num_read - 1 - i];
          in_buf[2 * (num_read - 1 - i) + 1] = in_buf[num_read - 1 - i];
        }
      }

      in_offset = 0;
    }
  }
  cur_frame = 0;
}

void Playhead::inc() {
  ++cur_frame;

  if (last_iteration && cur_frame >= out_offset)
    state = FINISHED;
}

void Playhead::get_values(double values[]) {
  values[0] = pitch_buf[2 * cur_frame];
  values[1] = pitch_buf[2 * cur_frame + 1];
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
