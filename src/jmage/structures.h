#ifndef JM_STRUCTS_H
#define JM_STRUCTS_H

#include <jack/jack.h>

typedef jack_default_audio_sample_t sample_t;

typedef struct jm_queue {
  volatile int head;
  volatile int tail;
  size_t el_size;
  size_t length;
  char* arr;
} jm_queue;

void jm_init_queue(jm_queue* jmq, size_t el_size, size_t length);
void jm_destroy_queue(jm_queue* jmq);
void jm_q_add(jm_queue* jmq, void* p);
void* jm_q_remove(jm_queue* jmq, void* p);
void* jm_q_head(jm_queue* jmq);
void* jm_q_inc_ptr(jm_queue* jmq, void* p);

struct playhead {
  int pitch;
  double amp;
  double speed;
  double position;
  int released;
  int note_off;
  int rel_time;
  sample_t* wave[2];
  jack_nframes_t wave_length;
};

typedef struct ph_list_el {
  struct playhead ph;
  struct ph_list_el* next;
  struct ph_list_el* prev;
} ph_list_el;

typedef struct playhead_list {
  ph_list_el* head;
  ph_list_el* tail;
  ph_list_el* arr;
  size_t length;
  size_t size;
  jm_queue unused;
} playhead_list;

void init_ph_list(playhead_list* phl, size_t length);
void destroy_ph_list(playhead_list* phl);
ph_list_el* ph_list_head(playhead_list* phl);
void ph_list_add(playhead_list* phl, struct playhead* ph);
void ph_list_remove(playhead_list* phl, ph_list_el* pel);
void ph_list_remove_last(playhead_list* phl);
size_t ph_list_size(playhead_list* phl);

struct key_zone {
  sample_t* wave[2];
  jack_nframes_t wave_length;
  int lower_bound;
  int upper_bound;
  int origin;
};

int in_zone(struct key_zone* z, int pitch);
void zone_to_ph(struct key_zone* zone, struct playhead* ph, int pitch, int velocity);

#endif
