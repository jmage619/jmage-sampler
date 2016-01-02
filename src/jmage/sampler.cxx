#include <stdexcept> 

#include <limits.h>
#include <stdlib.h>
#include <sndfile.h>
#include <jack/jack.h>
#include <jack/types.h>

#include "jmage/sampler.h"
#include "jmage/jmsampler.h"

void jm_parse_wave(jm_wave* wav, char const * path) {
  SF_INFO sf_info;
  sf_info.format = 0;
  SNDFILE* sf_wav = sf_open(path, SFM_READ, &sf_info);
  wav->length = sf_info.frames;
  wav->num_channels = sf_info.channels;
  wav->wave = new float[wav->num_channels * wav->length];
  sf_read_float(sf_wav, wav->wave, wav->num_channels * wav->length);

  SF_INSTRUMENT inst;
  sf_command(sf_wav, SFC_GET_INSTRUMENT, &inst, sizeof(inst));

  wav->has_loop = 0;
  wav->left = 0;
  wav->right = wav->length;

  // if the wav has loops, just pick its first one(?)
  // also ignoring loop mode and count for now
  if (inst.loop_count > 0) {
    wav->has_loop = 1;
    wav->left = inst.loops[0].start;
    wav->right = inst.loops[0].end;
  }

  sf_close(sf_wav);
}

void jm_destroy_wave(jm_wave* wav) {
  delete [] wav->wave;
}

// why aren't we initializing zone->right
void jm_init_key_zone(jm_key_zone* zone) {
  zone->start = 0;
  zone->left = 0;
  zone->lower_bound = NOTE_MIN;
  zone->upper_bound = NOTE_MAX;
  zone->origin = ORIGIN_DEFAULT;
  zone->amp = 1.0;
  zone->attack = 0;
  zone->hold = 0;
  zone->decay = 0;
  zone->sustain = 1.0;
  zone->release = 0;
  zone->pitch_corr = 0.0;
  zone->loop_on = 0;
  zone->crossfade = 0;
}

int jm_zone_contains(jm_key_zone const * zone, int pitch) {
  if (pitch >= zone->lower_bound && pitch <= zone->upper_bound)
    return 1;
  return 0;
}

JMSampler* jm_new_sampler() {
  try { 
    return new JMSampler();
  }
  catch (std::runtime_error& e) {
    return NULL;
  }
}

void jm_destroy_sampler(JMSampler* jms) {
  delete jms;
}

void jm_add_zone(JMSampler* jms, jm_key_zone const * zone) {
  jms->add_zone(*zone);
}

void jm_get_zone(JMSampler* jms, int index, jm_key_zone* zone) {
  *zone = jms->get_zone(index);
}

void jm_update_zone(JMSampler* jms, int index, jm_key_zone const * zone) {
  jms->update_zone(index, *zone);
}

void jm_remove_zone(JMSampler* jms, int index) {
  jms->remove_zone(index);
}

size_t jm_num_zones(JMSampler* jms){
  return jms->num_zones();
}

void jm_send_msg(JMSampler* jms, jm_msg const * msg) {
  jms->send_msg(*msg);
}

int jm_receive_msg(JMSampler* jms, jm_msg* msg) {
  return jms->receive_msg(*msg);
}
