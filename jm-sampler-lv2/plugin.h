#ifndef PLUGIN_H
#define PLUGIN_H

#include <vector>
#include <map>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#include <lib/zone.h>
#include <lib/lv2_uris.h>
#include <lib/sfzparser.h>
#include <lib/jmsampler.h>

namespace jm {
  struct sampler_plugin {
    int sample_rate;
    const LV2_Atom_Sequence* control_port;
    LV2_Atom_Sequence* notify_port;
    float* out1;
    float* out2;
    LV2_URID_Map* map;
    jm::uris uris;
    LV2_Worker_Schedule* schedule;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame seq_frame;
    int zone_number; // only for naming
    sfz::sfz* patch;
    std::vector<jm_zone> zones;
    std::map<std::string, jm::wave> waves;
    char patch_path[256];
    char wav_path[256];
    float* channel;
    float* level;
    JMSampler* sampler;
  };

  void send_sample_rate(sampler_plugin* plugin);
  void send_add_zone(sampler_plugin* plugin, int index, const jm_zone& zone);
  void send_remove_zone(sampler_plugin* plugin, int index);
  void send_update_vol(sampler_plugin* plugin, float vol);
  void send_update_chan(sampler_plugin* plugin, float chan);
  void update_zone(sampler_plugin* plugin, const LV2_Atom_Object* obj);
  void add_zone_from_wave(sampler_plugin* plugin, int index, const char* path);
  void add_zone_from_region(sampler_plugin* plugin, const std::map<std::string, SFZValue>& region);
};

#endif
