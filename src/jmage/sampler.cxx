#include <limits.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <jack/types.h>
#include "jmage/sampler.h"
#include "jmage/control.h"

jm_key_zone* jm_zones;
int jm_num_zones; 

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

jack_client_t* jm_init_sampler() {
  // init amplitude array
  ctrl::init_amp();

  // init jack
  jack_client_t* client;
  jack_status_t status; 
  if ((client = jack_client_open("ghetto_sampler", JackNullOption, &status)) == NULL) {
    //fprintf (stderr, "jack server not running?\n");
    return NULL;
  }

  jack_set_process_callback(client, ctrl::process_callback, 0);
  ctrl::input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
  ctrl::output_port1 = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  ctrl::output_port2 = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  if (jack_activate(client)) {
    //fprintf (stderr, "cannot activate client");
    return NULL;
  }
  return client;
}

void jm_destroy_sampler(jack_client_t *client) {
  jack_deactivate(client);
  jack_client_close(client);

  // clean up any lingering messages
  jm_msg* msg;
  while (ctrl::msg_q_in.remove(msg)) {
    jm_destroy_msg(msg);
  }
}

jm_msg* jm_new_msg() {
  return new jm_msg;
}

void jm_destroy_msg(jm_msg* msg) {
  delete msg;
}

void jm_send_msg(jm_msg* msg) {
  ctrl::msg_q_in.add(msg);
}

int jm_receive_msg(jm_msg** msg) {
  if (ctrl::msg_q_out.remove(*msg))
    return 1;
  return 0;
}
