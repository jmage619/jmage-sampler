#include <cstdlib>
#include <cstdio>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#include "jmage/jmsampler.h"

#define JM_SAMPLER_URI "http://lv2plug.in/plugins/jm-sampler"

enum {
  SAMPLER_CONTROL = 0,
  SAMPLER_NOTIFY  = 1,
  SAMPLER_OUT_L = 2,
  SAMPLER_OUT_R = 3
};

struct jm_uris {
  LV2_URID midi_Event;
};

struct jm_sampler_plugin {
  const LV2_Atom_Sequence* control_port;
  LV2_Atom_Sequence* notify_port;
  float* out_port_l;
  float* out_port_r;
  LV2_URID_Map* map;
  jm_uris uris;
  JMSampler sampler;
};

static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
    double rate, const char* path, const LV2_Feature* const* features) {
  jm_sampler_plugin* plugin = new jm_sampler_plugin;

  // Scan host features for URID map
  LV2_URID_Map* map = NULL;
  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map)) {
      map = static_cast<LV2_URID_Map*>(features[i]->data);
    }
  }
  if (!map) {
    fprintf(stderr, "Host does not support urid:map.\n");
    delete plugin;
    return NULL;
  }

  // Map URIS
  plugin->map = map;
  plugin->uris.midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);

  printf("sampler instantiated.\n");
  return plugin;
}

static void cleanup(LV2_Handle instance)
{
  delete static_cast<jm_sampler_plugin*>(instance);
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);
  switch (port) {
    case SAMPLER_CONTROL:
      plugin->control_port = static_cast<const LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_NOTIFY:
      plugin->notify_port = static_cast<LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_OUT_L:
      plugin->out_port_l = (float*) data;
      break;
    case SAMPLER_OUT_R:
      plugin->out_port_r = (float*) data;
      break;
    default:
      break;
  }
}

static void run(LV2_Handle instance, uint32_t n_samples) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

  LV2_ATOM_SEQUENCE_FOREACH(plugin->control_port, ev) {
    if (ev->body.type == plugin->uris.midi_Event) {
      const uint8_t* const msg = (const uint8_t*)(ev + 1);
      switch (lv2_midi_message_type(msg)) {
        case LV2_MIDI_MSG_NOTE_ON:
            printf("midi note on; note: %i; vel: %i\n", msg[1], msg[2]);
          break;
        default:
          break;
      }
    }
  }
}

static const LV2_Descriptor descriptor = {
  JM_SAMPLER_URI,
  instantiate,
  connect_port,
  NULL, // activate
  run,
  NULL, // deactivate
  cleanup,
  NULL // extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
  switch (index) {
    case 0:
      return &descriptor;
    default:
      return NULL;
  }
}
