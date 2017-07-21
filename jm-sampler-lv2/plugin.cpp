#include <cstdio>
#include <cmath>
#include <map>
#include <vector>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>

#include <lib/zone.h>
#include <lib/wave.h>
#include <lib/sfzparser.h>
#include <lib/lv2_uris.h>

#include "plugin.h"

void jm::send_zone_vect(sampler_plugin* plugin) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_getZoneVect);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_long(&plugin->forge, reinterpret_cast<long>(&plugin->zones));
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: zone vector sent!!%li\n", &plugin->zones);
}

void jm::send_sample_rate(sampler_plugin* plugin) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_getSampleRate);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_int(&plugin->forge, plugin->sample_rate);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: sample rate sent!! %i\n", plugin->sample_rate);
}

void jm::send_add_zone(sampler_plugin* plugin, int index) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_addZone);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_int(&plugin->forge, index);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: add zone sent!! %i: %s\n", index, plugin->zones[index].name);
}

void jm::send_remove_zone(sampler_plugin* plugin, int index) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_removeZone);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_int(&plugin->forge, index);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: remove zone sent!! %i\n", index);
}

void jm::send_clear_zones(sampler_plugin* plugin) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_clearZones);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: clear zones sent!!\n");
}

void jm::send_update_vol(sampler_plugin* plugin, float vol) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_updateVol);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_float(&plugin->forge, vol);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update vol sent!! %f\n", vol);
}

void jm::send_update_chan(sampler_plugin* plugin, float chan) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_updateChan);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_float(&plugin->forge, chan);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update chan sent!! %f\n", chan);
}

void jm::update_zone(sampler_plugin* plugin, const LV2_Atom_Object* obj) {
  LV2_Atom* params = NULL;

  lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
  LV2_Atom* a = lv2_atom_tuple_begin(reinterpret_cast<LV2_Atom_Tuple*>(params));
  int index = reinterpret_cast<LV2_Atom_Int*>(a)->body;
  a = lv2_atom_tuple_next(a);
  int type = reinterpret_cast<LV2_Atom_Int*>(a)->body;
  a = lv2_atom_tuple_next(a);
  switch (type) {
    case ZONE_NAME:
      strcpy(plugin->zones.at(index).name, (char*)(a + 1));
      break;
    case ZONE_AMP:
      plugin->zones.at(index).amp = reinterpret_cast<LV2_Atom_Float*>(a)->body;
      break;
    case ZONE_ORIGIN:
      plugin->zones.at(index).origin = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_LOW_KEY:
      plugin->zones.at(index).low_key = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_HIGH_KEY:
      plugin->zones.at(index).high_key = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_LOW_VEL:
      plugin->zones.at(index).low_vel = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_HIGH_VEL:
      plugin->zones.at(index).high_vel = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_PITCH:
      plugin->zones.at(index).pitch_corr = reinterpret_cast<LV2_Atom_Double*>(a)->body;
      break;
    case ZONE_START:
      plugin->zones.at(index).start = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_LEFT:
      plugin->zones.at(index).left = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_RIGHT:
      plugin->zones.at(index).right = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_LOOP_MODE:
      plugin->zones.at(index).loop_mode = (jm::loop_mode) reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_CROSSFADE:
      plugin->zones.at(index).crossfade = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_GROUP:
      plugin->zones.at(index).group = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_OFF_GROUP:
      plugin->zones.at(index).off_group = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_ATTACK:
      plugin->zones.at(index).attack = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_HOLD:
      plugin->zones.at(index).hold = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_DECAY:
      plugin->zones.at(index).decay = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_SUSTAIN:
      plugin->zones.at(index).sustain = reinterpret_cast<LV2_Atom_Float*>(a)->body;
      break;
    case ZONE_RELEASE:
      plugin->zones.at(index).release = reinterpret_cast<LV2_Atom_Int*>(a)->body;
      break;
    case ZONE_PATH:
      strcpy(plugin->zones.at(index).path, (char*)(a + 1));
      break;
  }
}

void jm::add_zone_from_wave(sampler_plugin* plugin, int index, const char* path) {
  jm::wave& wav = plugin->waves[path];
  jm::zone zone;
  jm::init_zone(&zone);
  zone.wave = wav.wave;
  zone.num_channels = wav.num_channels;
  zone.sample_rate = wav.sample_rate;
  zone.wave_length = wav.length;
  zone.left = wav.left;
  zone.right = wav.length;
  if (wav.has_loop)
    zone.loop_mode = LOOP_CONTINUOUS;
  sprintf(zone.name, "Zone %i", plugin->zone_number++);
  strcpy(zone.path, path);
  if (index >= 0) {
    plugin->zones.insert(plugin->zones.begin() + index, zone);
    send_add_zone(plugin, index);
  }
  else {
    plugin->zones.push_back(zone);
    send_add_zone(plugin, plugin->zones.size() - 1);
  }
}

void jm::add_zone_from_region(sampler_plugin* plugin, const std::map<std::string, SFZValue>& region) {
  jm::wave& wav = plugin->waves[region.find("sample")->second.get_str()];
  jm::zone zone;
  jm::init_zone(&zone);
  zone.wave = wav.wave;
  zone.num_channels = wav.num_channels;
  zone.sample_rate = wav.sample_rate;
  zone.wave_length = wav.length;
  zone.left = wav.left;
  zone.right = wav.length;
  if (wav.has_loop)
    zone.loop_mode = LOOP_CONTINUOUS;

  std::map<std::string, SFZValue>::const_iterator it = region.find("jm_name");
  if (it != region.end())
    strcpy(zone.name, it->second.get_str().c_str());
  else
    sprintf(zone.name, "Zone %i", plugin->zone_number++);

  strcpy(zone.path, region.find("sample")->second.get_str().c_str());
  zone.amp = pow(10., region.find("volume")->second.get_double() / 20.);
  zone.low_key = region.find("lokey")->second.get_int();
  zone.high_key = region.find("hikey")->second.get_int();
  zone.origin = region.find("pitch_keycenter")->second.get_int();
  zone.low_vel = region.find("lovel")->second.get_int();
  zone.high_vel = region.find("hivel")->second.get_int();
  zone.pitch_corr = region.find("tune")->second.get_int() / 100.;
  zone.start = region.find("offset")->second.get_int();

  jm::loop_mode mode = (jm::loop_mode) region.find("loop_mode")->second.get_int();
  if (mode != LOOP_UNSET)
    zone.loop_mode = mode;

  int loop_start = region.find("loop_start")->second.get_int();
  if (loop_start >= 0)
    zone.left = loop_start;

  int loop_end = region.find("loop_end")->second.get_int();
  if (loop_end >= 0)
    zone.right = loop_end;

  zone.crossfade = plugin->sample_rate * region.find("loop_crossfade")->second.get_double();
  zone.group = region.find("group")->second.get_int();
  zone.off_group = region.find("off_group")->second.get_int();
  zone.attack = plugin->sample_rate * region.find("ampeg_attack")->second.get_double();
  zone.hold = plugin->sample_rate * region.find("ampeg_hold")->second.get_double();
  zone.decay = plugin->sample_rate * region.find("ampeg_decay")->second.get_double();
  zone.sustain = region.find("ampeg_sustain")->second.get_double() / 100.;
  zone.release = plugin->sample_rate * region.find("ampeg_release")->second.get_double();
  plugin->zones.push_back(zone);

  send_add_zone(plugin, plugin->zones.size() - 1);
}
