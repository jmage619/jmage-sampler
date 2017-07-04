#include <cstdio>
#include <climits>
#include <cmath>
#include <cstring>

#include <samplerate.h>

#include "zone.h"
#include "components.h"

#define MAX_VELOCITY 127
// boost for controllers that don't reach 127 easily
//#define VELOCITY_BOOST 1.2f
#define VELOCITY_BOOST 1.0f

void AudioStream::init(const jm::zone& zone) {
  loop_on = (zone.loop_mode == jm::LOOP_CONTINUOUS) ? true : false;
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

int AudioStream::read(float* buf, int nframes) {
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

      memcpy(buf + out_offset, wave + num_channels * cur_frame, num_channels * to_copy * sizeof(float));
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
          buf[out_offset] = 0.0f;
          if (cur_frame < wave_length)
            buf[out_offset] +=  (1.0f - cf_timer / (float) crossfade) * wave[cur_frame];
          if (fade_in_start + cf_timer >= 0) {
            buf[out_offset] +=  cf_timer / (float) crossfade * wave[fade_in_start + cf_timer];
          }
        }
        else {
          //printf("cf1: %f,%f\n",(1.0 - cf_timer / (float) crossfade),cf_timer / (float) crossfade);
          buf[out_offset] = 0.0f;
          buf[out_offset + 1] = 0.0f;
          if (cur_frame < wave_length) {
            buf[out_offset] += (1.0f - cf_timer / (float) crossfade) * wave[num_channels * cur_frame];
            buf[out_offset + 1] += (1.0f - cf_timer / (float) crossfade) * wave[num_channels * cur_frame + 1];
          }
          if (fade_in_start + cf_timer >= 0) {
            buf[out_offset] += cf_timer / (float) crossfade * wave[num_channels * fade_in_start + num_channels * cf_timer];
            buf[out_offset + 1] += cf_timer / (float) crossfade * wave[num_channels * fade_in_start + num_channels * cf_timer + 1];
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
    
    memcpy(buf, wave + num_channels * cur_frame, num_channels * to_copy * sizeof(float));
    cur_frame += to_copy;
    return to_copy;
  }
}

Playhead::Playhead(JMStack<Playhead*>& playhead_pool, int sample_rate, size_t in_nframes, size_t out_nframes):
    playhead_pool(playhead_pool), sample_rate(sample_rate), in_nframes(in_nframes) {
  // buf size * 2 to make room for stereo
  in_buf = new float[in_nframes * 2];
  out_buf = new float[out_nframes * 2];
  int error;
  // have to always make it stereo since they are allocated in advance
  //resampler = src_new(SRC_SINC_FASTEST, 2, &error);
  //resampler = src_new(SRC_ZERO_ORDER_HOLD, 2, &error);
  resampler = src_new(SRC_LINEAR, 2, &error);
}

Playhead::~Playhead() {
  src_delete(resampler);
  delete [] in_buf;
  delete [] out_buf;
}

void Playhead::init(const jm::zone& zone, int pitch) {
  src_ratio = sample_rate / (double) zone.sample_rate;
  SoundGenerator::init(zone, pitch);
  as.init(zone);
  num_channels = zone.num_channels;
  state = PLAYING;
  speed = pow(2, (pitch + zone.pitch_corr - zone.origin) / 12.);
  in_offset = 0;

  num_read = as.read(in_buf, in_nframes);
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

void Playhead::pre_process(size_t nframes) {
  memset(out_buf, 0,  2 * nframes * sizeof(float));

  out_offset = 0;

  SRC_DATA data;
  data.src_ratio = 1 / speed * src_ratio;
  //data.src_ratio = 1 / speed;
  data.end_of_input = 0;

  while (out_offset < nframes) {
    data.data_in = in_buf + 2 * in_offset;
    data.input_frames = num_read - in_offset;
    data.data_out = out_buf + 2 * out_offset;
    data.output_frames = nframes - out_offset;

    src_process(resampler, &data);
    out_offset += data.output_frames_gen;
    in_offset += data.input_frames_used;

    /* replace above 3 lines with this to compare pitch shift against pass through
    int to_read = data.input_frames < data.output_frames ? data.input_frames : data.output_frames;
    memcpy(data.data_out, data.data_in, 2 * to_read * sizeof(float));
    out_offset += to_read;
    in_offset += to_read;
    */

    if (in_offset >= num_read) {
      num_read = as.read(in_buf, in_nframes);

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

void Playhead::get_values(float* values) {
  values[0] = out_buf[2 * cur_frame];
  values[1] = out_buf[2 * cur_frame + 1];
}

void AmpEnvGenerator::init(SoundGenerator* sg, const jm::zone& zone, int pitch, int velocity) {
  SoundGenerator::init(zone, pitch);
  this->sg = sg;
  state = ATTACK;
  attack = zone.attack;
  hold = zone.hold;
  decay = zone.decay;
  sustain = zone.sustain;
  release = zone.release;
  timer = 0;
  env_rel_val = zone.sustain;
  float calc_amp = zone.amp * VELOCITY_BOOST * velocity / (float) MAX_VELOCITY;
  amp = calc_amp > 1.0f ? 1.0f : calc_amp;
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

float AmpEnvGenerator::get_env_val() {
  switch (state) {
    case ATTACK:
      // envelope from 0.0 to 1.0; linear feels better here
      if (attack != 0)
        return timer / (float) attack;
      /*if (attack != 0)
        return 0.00001f * pow(10.f, 5.0f * timer / (float) attack);
      */
      break;
    case DECAY:
      // envelope from 1.0 to sustain
      /*if (decay != 0)
        return amp * (-(1.0f - sustain) * timer / decay + 1.0f);
      */
      // decay rate 1.0 to 0.0; stop when we hit sustain
      /*if (decay != 0) {
        float calc_val = 1.0f - timer / (float) decay;
        calc_val = calc_val < sustain ? sustain: calc_val;
        return calc_val;
      }
      */
      if (decay != 0) {
        float calc_val = 0.00001f * pow(10.f, 5.0f * (1.0f - timer / (float) decay));
        calc_val = calc_val < sustain ? sustain: calc_val;
        return calc_val;
      }
      break;
    case SUSTAIN:
      return sustain;
    case RELEASE:
      // from rel_amp to 0.0
      /*if (release != 0)
        return - rel_amp * timer / release + rel_amp;
      else
        return rel_amp;
      */
      // release rate 1.0 to 0.0; start from released value and stop when hit 0.0
      /*if (release != 0) {
        float calc_val = env_rel_val - timer / (float) release;
        calc_val = calc_val < 0.0f ? 0.0f: calc_val;
        return calc_val;
      }
      else
        return env_rel_val;
      */
      if (release != 0) {
        float calc_val = env_rel_val * 0.00001f * pow(10.f, 5.0f * (1.0f - timer / (float) release));
        //calc_val = calc_val < 0.0f ? 0.0f: calc_val;
        return calc_val;
      }
      else
        return env_rel_val;
    default:
      break;
  }
  return 1.0f;
}

void AmpEnvGenerator::get_values(float* values) {
  float cur_env = get_env_val();
  sg->get_values(values);
  values[0] *= cur_env * amp;
  values[1] *= cur_env * amp;
}

void AmpEnvGenerator::set_release() {
  env_rel_val = get_env_val();
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

  while (unused.size() > 0) {
    delete unused.pop();
  }
}

void SoundGenList::add(SoundGenerator* sg) {
  sg_list_el* sg_el = unused.pop();
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
