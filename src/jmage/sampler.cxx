#include <stdexcept> 

#include <limits.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <jack/types.h>

#include <stdexcept> 
#include "jmage/sampler.h"
#include "jmage/jmsampler.h"

// why aren't we initializing zone->right
void jm_init_key_zone(jm_key_zone* zone) {
  zone->start = 0;
  zone->left = 0;
  zone->lower_bound = INT_MIN;
  zone->upper_bound = INT_MAX;
  zone->origin = 48;
  zone->amp = 1.0;
  zone->rel_time = 0.0;
  zone->pitch_corr = 0.0;
  zone->loop_on = 0;
  zone->crossfade = 0;
}

int jm_zone_contains(jm_key_zone* zone, int pitch) {
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

void jm_add_zone(JMSampler* jms, int key, jm_key_zone* zone) {
  jms->add_zone(key, *zone);
}

void jm_remove_zone(JMSampler* jms, int key) {
  jms->remove_zone(key);
}

void jm_send_msg(JMSampler* jms, jm_msg* msg) {
  jms->send_msg(*msg);
}

int jm_receive_msg(JMSampler* jms, jm_msg* msg) {
  return jms->receive_msg(*msg);
}
