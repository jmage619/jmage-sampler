#include <cstdio>
#include <cmath>
#include <map>
#include <vector>
#include <pthread.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>

#include <lib/zone.h>
#include <lib/wave.h>
#include <lib/sfzparser.h>
#include <lib/lv2_uris.h>

#include "lv2sampler.h"

void LV2Sampler::send_sample_rate() {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_getSampleRate);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_int(&forge, sample_rate);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: sample rate sent!! %i\n", sample_rate);
}

void LV2Sampler::send_zone_vect() {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_getZoneVect);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_long(&forge, reinterpret_cast<long>(&zones));
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: zone vector sent!!%li\n", reinterpret_cast<long>(&zones));
}

void LV2Sampler::send_add_zone(int index) {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_addZone);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_int(&forge, index);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: add zone sent!! %i: %s\n", index, zones[index].name);
}

void LV2Sampler::send_update_wave(int index) {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_updateWave);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_int(&forge, index);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update wave sent!! %i: %s\n", index, zones[index].path);
}

void LV2Sampler::send_remove_zone(int index) {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_removeZone);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_int(&forge, index);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: remove zone sent!! %i\n", index);
}

void LV2Sampler::send_clear_zones() {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_clearZones);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: clear zones sent!!\n");
}

void LV2Sampler::send_update_vol(float vol) {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_updateVol);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_float(&forge, vol);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update vol sent!! %f\n", vol);
}

void LV2Sampler::send_update_chan(float chan) {
  lv2_atom_forge_frame_time(&forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&forge, &obj_frame, 0, uris.jm_updateChan);
  lv2_atom_forge_key(&forge, uris.jm_params);
  lv2_atom_forge_float(&forge, chan);
  lv2_atom_forge_pop(&forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update chan sent!! %f\n", chan);
}
