#ifndef JM_CONTROL_H
#define JM_CONTROL_H

#include <jack/types.h>

#include "jmage/sampler.h"
#include "jmage/collections.h"

namespace ctrl {
  extern jack_port_t *input_port;
  extern jack_port_t *output_port1;
  extern jack_port_t *output_port2;
  extern JMQueue<jm_msg*> msg_q_in;
  extern JMQueue<jm_msg*> msg_q_out;

  void init_amp();
  int process_callback(jack_nframes_t nframes, void *arg);
}

#endif
