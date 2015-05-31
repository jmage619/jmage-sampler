#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jack/jack.h>
#include "jmage/structures.h"

#define MAX_VELOCITY 127
// queue functions

void jm_init_queue(jm_queue* jmq, size_t el_size, size_t length) {
  jmq->head = 0;
  jmq->tail = 0;
  jmq->el_size = el_size;
  jmq->length = length + 1;
  jmq->arr = malloc(jmq->el_size * jmq->length);
  jmq->size = 0;
} 

void jm_destroy_queue(jm_queue* jmq) {
  free(jmq->arr);
}

void jm_q_add(jm_queue* jmq, void* p) {
  memcpy(jmq->arr + jmq->tail * jmq->el_size, p, jmq->el_size);
  jmq->tail = (jmq->tail + 1) % jmq->length;
  jmq->size++;
}

void* jm_q_remove(jm_queue* jmq, void* p) {
  if (jmq->head == jmq->tail)
    return NULL;

  if (p != NULL)
    memcpy(p, jmq->arr + jmq->head * jmq->el_size, jmq->el_size);

  jmq->head = (jmq->head + 1) % jmq->length;
  jmq->size--;
  return p;
}

void* jm_q_head(jm_queue* jmq) {
  if (jmq->head == jmq->tail)
    return NULL;
  return jmq->arr + jmq->head * jmq->el_size;
}

size_t jm_q_size(jm_queue* jmq) {
  return jmq->size;
}

void* jm_q_inc_ptr(jm_queue* jmq, void* p) {
  int new_off = (((char*)p - jmq->arr) / jmq->el_size + 1) % jmq->length;
  if (new_off == jmq->tail)
    return NULL;
  return jmq->arr + new_off * jmq->el_size;
}

// ph list functions

void init_ph_list(playhead_list* phl, size_t length) {
  phl->head = NULL;
  phl->tail = NULL;
  phl->length = length;
  phl->size = 0;
  phl->arr = malloc(sizeof(ph_list_el) * phl->length);
  jm_init_queue(&phl->unused, sizeof(ph_list_el*), phl->length);
  size_t i;
  for (i = 0; i < phl->length; i++) {
    ph_list_el* pel = phl->arr + i;
    jm_q_add(&phl->unused, &pel);
  }
}

void destroy_ph_list(playhead_list* phl) {
  jm_destroy_queue(&phl->unused);
  free(phl->arr);
}

ph_list_el* ph_list_head(playhead_list* phl) {
  return phl->head;
}

void ph_list_add(playhead_list* phl, struct playhead* ph) {
  ph_list_el* pel;
  jm_q_remove(&phl->unused, &pel);
  pel->ph = *ph;
  pel->next = phl->head;
  pel->prev = NULL;
  if (pel->next == NULL)
    phl->tail = pel;
  else
    pel->next->prev = pel;

  phl->head = pel;
  phl->size++;
}

int ph_list_in(playhead_list* phl, ph_list_el* pel) {
  ph_list_el* p;
  for (p = phl->head; p != NULL; p = p->next) {
    if (p == pel)
      return 1;
  }
  return 0;
}

void ph_list_remove(playhead_list* phl, ph_list_el* pel) {
  jm_q_add(&phl->unused, &pel);

  if (pel == phl->head)
    phl->head = phl->head->next;
  else
    pel->prev->next = pel->next;

  if (pel == phl->tail)
    phl->tail = phl->tail->prev;
  else
    pel->next->prev = pel->prev;

  phl->size--;
}

void ph_list_remove_last(playhead_list* phl) {
  ph_list_remove(phl, phl->tail);
}

size_t ph_list_size(playhead_list* phl) {
  return phl->size;
}

int in_zone(struct key_zone* z, int pitch) {
  if (pitch >= z->lower_bound && pitch <= z->upper_bound)
    return 1;
  return 0;
}

void zone_to_ph(struct key_zone* zone, struct playhead* ph, int pitch, int velocity) {
  ph->pitch = pitch;
  // 1.5 boost helps oxygen 8 go to 1.0
  double calc_amp = 1.5 * velocity / MAX_VELOCITY;
  ph->amp = calc_amp > 1.0 ? 1.0 : calc_amp;
  ph->position = 0;
  ph->speed = pow(2, (pitch - zone->origin) / 12.);
  ph->wave[0] = zone->wave[0];
  ph->wave[1] = zone->wave[1];
}
