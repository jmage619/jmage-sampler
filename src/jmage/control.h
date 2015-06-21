#ifndef JM_CONTROL_H
#define JM_CONTROL_H

#include <jack/types.h>

extern jack_port_t *input_port;
extern jack_port_t *output_port1;
extern jack_port_t *output_port2;

void jm_init_amp();
int jm_process_audio(jack_nframes_t nframes, void *arg);

#endif
