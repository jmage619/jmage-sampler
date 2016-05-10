#include <stdexcept> 

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sndfile.h>
#include <jack/jack.h>
#include <jack/types.h>

#include "jmage/sampler.h"
#include "jmage/jmsampler.h"

int jm_parse_wave(jm_wave* wav, char const * path) {
  SF_INFO sf_info;
  sf_info.format = 0;
  SNDFILE* sf_wav = sf_open(path, SFM_READ, &sf_info);
  if (sf_wav == NULL) {
    fprintf(stderr, "error opening %s: %s\n", path, sf_strerror(NULL));
    return 1;
  }
  wav->length = sf_info.frames;
  wav->num_channels = sf_info.channels;
  wav->wave = new float[wav->num_channels * wav->length];
  sf_read_float(sf_wav, wav->wave, wav->num_channels * wav->length);

  wav->has_loop = 0;
  wav->left = 0;
  wav->right = wav->length;

  SF_INSTRUMENT inst;
  if (sf_command(sf_wav, SFC_GET_INSTRUMENT, &inst, sizeof(inst)) == SF_FALSE) {
    printf("wav %s: no instrument info found, assuming 0 loop points\n", path);
    sf_close(sf_wav);
    return 0;
  } 

  // if the wav has loops, just pick its first one(?)
  // also ignoring loop mode and count for now
  if (inst.loop_count > 0) {
    wav->has_loop = 1;
    wav->left = inst.loops[0].start;
    wav->right = inst.loops[0].end;
  }

  sf_close(sf_wav);
  return 0;
}

void jm_destroy_wave(jm_wave* wav) {
  delete [] wav->wave;
}

// why aren't we initializing zone->right
void jm_init_key_zone(jm_key_zone* zone) {
  zone->start = 0;
  zone->left = 0;
  zone->low_key = NOTE_MIN;
  zone->high_key = NOTE_MAX;
  zone->origin = ORIGIN_DEFAULT;
  zone->low_vel = VEL_MIN;
  zone->high_vel = VEL_MAX;
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

void jm_zone_set_name(jm_key_zone* zone, char const * name) {
  strcpy(zone->name, name);
}

void jm_zone_set_path(jm_key_zone* zone, char const * path) {
  strcpy(zone->path, path);
}

int jm_zone_contains(jm_key_zone const * zone, int pitch, int velocity) {
  if (pitch >= zone->low_key && pitch <= zone->high_key
      && velocity >= zone->low_vel && velocity <= zone->high_vel)
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
