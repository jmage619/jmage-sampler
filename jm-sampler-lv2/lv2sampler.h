#ifndef LV2SAMPLER_H
#define LV2SAMPLER_H

#include <vector>
#include <map>
#include <pthread.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>

#include <lib/zone.h>
#include <lib/lv2_uris.h>
#include <lib/sfzparser.h>
#include <lib/jmsampler.h>

class LV2Sampler: public JMSampler {
  public:
    const LV2_Atom_Sequence* control_port;
    LV2_Atom_Sequence* notify_port;
    float* out1;
    float* out2;
    LV2_URID_Map* map;
    jm::uris uris;
    LV2_Worker_Schedule* schedule;
    LV2_Atom_Forge forge;
    LV2_Atom_Forge_Frame seq_frame;
    char wav_path[256];
    char patch_path[256];

    LV2Sampler(int sample_rate, size_t in_nframes, size_t out_nframes):
      JMSampler(sample_rate, in_nframes, out_nframes) {patch_path[0] = '\0';};
    void send_sample_rate();
    void send_zone_vect();
    void send_add_zone(int index);
    void send_remove_zone(int index);
    void send_clear_zones();
    void send_update_vol(float vol);
    void send_update_chan(float chan);
};

#endif
