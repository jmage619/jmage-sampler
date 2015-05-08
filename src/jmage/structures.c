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

void ph_list_add(playhead_list* phl, struct playhead* ph) {
  struct ph_list_el* pel;
  jm_q_remove(&phl->unused, &pel);
  pel->ph = *ph;
  pel->next = phl->head;
  phl->head = pel;
  phl->size++;
}

size_t ph_list_size(playhead_list* phl) {
  return phl->size;
}

void init_ph_list_iterator(playhead_list* phl, ph_list_iterator* phl_it) {
  phl_it->p = phl->head;
  phl_it->prev = NULL;
  phl_it->prev_prev = NULL;
  phl_it->phl = phl;
}

struct playhead* ph_list_iter_next(ph_list_iterator* phl_it) {
  if(phl_it->p == NULL)
    return NULL;

  struct playhead* ph = &phl_it->p->ph;
  phl_it->prev_prev = phl_it->prev;
  phl_it->prev = phl_it->p;
  phl_it->p = phl_it->p->next;
  return ph;
}

void ph_list_iter_remove(ph_list_iterator* phl_it) {
  jm_q_add(&phl_it->phl->unused, &phl_it->prev);
  if (phl_it->prev == phl_it->phl->head) {
    phl_it->phl->head = phl_it->phl->head->next;
    phl_it->prev = NULL;
    phl_it->phl->size--;
    return;
  }
  phl_it->prev_prev->next = phl_it->prev->next;
  phl_it->prev = phl_it->prev_prev;
  phl_it->phl->size--;
}

int in_zone(struct key_zone z, int pitch) {
  if (pitch >= z.lower_bound && pitch <= z.upper_bound)
    return 1;
  return 0;
}

struct playhead zone_to_ph(struct key_zone zones[], int length, int pitch) {
  struct playhead ph;
  int i;
  for (i = 0; i < length; i++) {
    if (in_zone(zones[i], pitch)) {
      struct playhead ph;
      ph.position = 0;
      ph.speed = pow(2, (pitch - zones[i].origin) / 12.);
      ph.wave[0] = zones[i].wave[0];
      ph.wave[1] = zones[i].wave[1];
      return ph;
    }
  }
  return ph;
}
