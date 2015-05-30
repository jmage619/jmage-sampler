#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <jack/jack.h>
#include "jmage/structures.h"

// queue functions

void jm_init_queue(jm_queue* jmq, size_t el_size, size_t length) {
  jmq->head = 0;
  jmq->tail = 0;
  jmq->el_size = el_size;
  jmq->length = length + 1;
  jmq->arr = malloc(jmq->el_size * jmq->length);
} 

void jm_destroy_queue(jm_queue* jmq) {
  free(jmq->arr);
}

void jm_q_add(jm_queue* jmq, void* p) {
  memcpy(jmq->arr + jmq->tail * jmq->el_size, p, jmq->el_size);
  jmq->tail = (jmq->tail + 1) % jmq->length;
}

void* jm_q_remove(jm_queue* jmq, void* p) {
  if (jmq->head == jmq->tail)
    return NULL;

  if (p != NULL)
    memcpy(p, jmq->arr + jmq->head * jmq->el_size, jmq->el_size);

  jmq->head = (jmq->head + 1) % jmq->length;
  return p;
}

void* jm_q_get_head(jm_queue* jmq) {
  if (jmq->head == jmq->tail)
    return NULL;
  return jmq->arr + jmq->head * jmq->el_size;
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
  phl->arr = malloc(sizeof(struct ph_list_el) * phl->length);
  jm_init_queue(&phl->unused, sizeof(struct ph_list_el*), phl->length);
  size_t i;
  for (i = 0; i < phl->length; i++) {
    struct ph_list_el* el_p = phl->arr + i;
    jm_q_add(&phl->unused, &el_p);
  }
}

void destroy_ph_list(playhead_list* phl) {
  jm_destroy_queue(&phl->unused);
  free(phl->arr);
}

void ph_list_add(playhead_list* phl, struct playhead ph) {
  struct ph_list_el* pel;
  jm_q_remove(&phl->unused, &pel);
  pel->ph = ph;
  pel->next = phl->head;
  pel->prev = NULL;
  if (pel->next == NULL)
    phl->tail = pel;
  else
    pel->next->prev = pel;

  phl->head = pel;
  phl->size++;
}

void ph_list_remove(playhead_list* phl, struct ph_list_el* pel) {
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

ph_list_iterator ph_list_get_iterator(playhead_list* phl) {
  ph_list_iterator it;
  it.p = phl->head;
  it.prev = NULL;
  it.phl = phl;

  return it;
}

struct playhead* ph_list_iter_next(ph_list_iterator* it) {
  if(it->p == NULL)
    return NULL;

  struct playhead* ph = &it->p->ph;
  it->prev = it->p;
  it->p = it->p->next;
  return ph;
}

void ph_list_iter_remove(ph_list_iterator* it) {
  ph_list_remove(it->phl, it->prev);
}

int in_zone(struct key_zone* z, int pitch) {
  if (pitch >= z->lower_bound && pitch <= z->upper_bound)
    return 1;
  return 0;
}

void zone_to_ph(struct key_zone* zone, struct playhead* ph, int pitch) {
  ph->position = 0;
  ph->speed = pow(2, (pitch - zone->origin) / 12.);
  ph->wave[0] = zone->wave[0];
  ph->wave[1] = zone->wave[1];
}
