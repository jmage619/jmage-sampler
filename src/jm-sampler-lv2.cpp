#include <cstdlib>
#include <cstdio>
#include <vector>
#include <cstring>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>

#include "uris.h"
#include "jmage/sampler.h"
#include "jmage/jmsampler.h"
#include "jmage/components.h"

enum {
  SAMPLER_CONTROL = 0,
  SAMPLER_NOTIFY  = 1,
  SAMPLER_OUT_L = 2,
  SAMPLER_OUT_R = 3
};

struct jm_sampler_plugin {
  const LV2_Atom_Sequence* control_port;
  LV2_Atom_Sequence* notify_port;
  float* out1;
  float* out2;
  LV2_URID_Map* map;
  jm_uris uris;
  LV2_Atom_Forge forge;
  JMSampler sampler;
};

static jm_wave WAV;

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
  jm_map_uris(plugin->map, &plugin->uris);
  lv2_atom_forge_init(&plugin->forge, plugin->map);

  jm_parse_wave(&WAV, "/home/jdost/dev/c/jmage-sampler/afx.wav");
  jm_key_zone zone;
  jm_init_key_zone(&zone);
  zone.wave = WAV.wave;
  zone.num_channels = WAV.num_channels;
  zone.wave_length = WAV.length;
  zone.right = WAV.length;
  strcpy(zone.name, "Zone 1");
  zone.amp = -0.921;
  zone.origin = 5;
  //zone.mode = LOOP_ONE_SHOT;
  plugin->sampler.zones_add(zone);

  jm_init_key_zone(&zone);
  zone.wave = WAV.wave;
  zone.num_channels = WAV.num_channels;
  zone.wave_length = WAV.length;
  zone.right = WAV.length;
  strcpy(zone.name, "Zone 2");
  zone.amp = -0.01;
  zone.origin = 3;
  plugin->sampler.zones_add(zone);

  printf("sampler instantiated.\n");
  return plugin;
}

static void cleanup(LV2_Handle instance)
{
  delete static_cast<jm_sampler_plugin*>(instance);
  jm_destroy_wave(&WAV);
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
      plugin->out1 = (float*) data;
      break;
    case SAMPLER_OUT_R:
      plugin->out2 = (float*) data;
      break;
    default:
      break;
  }
}

static void send_add_zone(jm_sampler_plugin* plugin, const jm_key_zone* zone) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_addZone);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  LV2_Atom_Forge_Frame tuple_frame;
  lv2_atom_forge_tuple(&plugin->forge, &tuple_frame);
  lv2_atom_forge_string(&plugin->forge, zone->name, strlen(zone->name));
  lv2_atom_forge_float(&plugin->forge, zone->amp);
  lv2_atom_forge_int(&plugin->forge, zone->origin);
  lv2_atom_forge_int(&plugin->forge, zone->low_key);
  lv2_atom_forge_int(&plugin->forge, zone->high_key);
  lv2_atom_forge_pop(&plugin->forge, &tuple_frame);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);
}

// later most of this opaque logic should be moved to member funs
// consider everything in common w/ stand alone jack audio callback when we re-implement that version
static void run(LV2_Handle instance, uint32_t n_samples) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

  memset(plugin->out1, 0, sizeof(uint32_t) * n_samples);
  memset(plugin->out2, 0, sizeof(uint32_t) * n_samples);

  // Set up forge to write directly to notify output port.
  const uint32_t notify_capacity = plugin->notify_port->atom.size;
  lv2_atom_forge_set_buffer(&plugin->forge, reinterpret_cast<uint8_t*>(plugin->notify_port),
    notify_capacity);

  LV2_Atom_Forge_Frame seq_frame;
  lv2_atom_forge_sequence_head(&plugin->forge, &seq_frame, 0);

  // we used to handle ui messages here
  // but now they will be in the same atom list as midi

  // must loop all atoms to find UI messages; not sure if qtractor bug
  // but they all occur at frame == n_samples (outside the range!)
  LV2_ATOM_SEQUENCE_FOREACH(plugin->control_port, ev) {
    if (ev->body.type == plugin->uris.atom_Object) {
      const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
      if (obj->body.otype == plugin->uris.jm_getZones) {
        //fprintf(stderr, "SAMPLER: get zones received!!\n");
        std::vector<jm_key_zone>::iterator it;
        for (it = plugin->sampler.zones_begin(); it != plugin->sampler.zones_end(); ++it) {
          send_add_zone(plugin, &*it);
        }
      }
      else if (obj->body.otype == plugin->uris.jm_updateZone) {
        //fprintf(stderr, "SAMPLER: update zone received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        LV2_Atom* a = lv2_atom_tuple_begin(reinterpret_cast<LV2_Atom_Tuple*>(params));
        int index = reinterpret_cast<LV2_Atom_Int*>(a)->body;
        a = lv2_atom_tuple_next(a);
        int type = reinterpret_cast<LV2_Atom_Int*>(a)->body;
        a = lv2_atom_tuple_next(a);
        switch (type) {
          case JM_ZONE_NAME:
            strcpy(plugin->sampler.zones_at(index).name, (char*)(a + 1));
            break;
          case JM_ZONE_AMP:
            plugin->sampler.zones_at(index).amp = reinterpret_cast<LV2_Atom_Float*>(a)->body;
            break;
          case JM_ZONE_ORIGIN:
            plugin->sampler.zones_at(index).origin = reinterpret_cast<LV2_Atom_Int*>(a)->body;
            break;
          case JM_ZONE_LOW_KEY:
            plugin->sampler.zones_at(index).low_key = reinterpret_cast<LV2_Atom_Int*>(a)->body;
            break;
          case JM_ZONE_HIGH_KEY:
            plugin->sampler.zones_at(index).high_key = reinterpret_cast<LV2_Atom_Int*>(a)->body;
            break;
        }
      }
    }
  }

  // pitch existing playheads
  sg_list_el* sg_el;
  for (sg_el = plugin->sampler.sound_gens_head(); sg_el != NULL; sg_el = sg_el->next) {
    sg_el->sg->pre_process(n_samples);
  }

  LV2_Atom_Event* ev = lv2_atom_sequence_begin(&plugin->control_port->body);

  // loop over frames in this callback window
  for (uint32_t n = 0; n < n_samples; ++n) {
    if (!lv2_atom_sequence_is_end(&plugin->control_port->body, plugin->control_port->atom.size, ev)) {
      // procces next event if it applies to current time (frame)
      while (n == ev->time.frames) {
        if (ev->body.type == plugin->uris.midi_Event) {
          const uint8_t* const msg = (const uint8_t*)(ev + 1);
          // only consider events from current channel
          if ((msg[0] & 0x0f) == plugin->sampler.channel) {
            // process note on
            if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_NOTE_ON) {
              // if sustain on and note is already playing, release old one first
              if (plugin->sampler.sustain_on) {
                for (sg_el = plugin->sampler.sound_gens_head(); sg_el != NULL; sg_el = sg_el->next) {
                  // doesn't apply to one shot
                  if (!sg_el->sg->one_shot && sg_el->sg->pitch == msg[1])
                    sg_el->sg->set_release();
                }
              }
              // pick out zones midi event matches against and add sound gens to queue
              std::vector<jm_key_zone>::iterator it;
              for (it = plugin->sampler.zones_begin(); it != plugin->sampler.zones_end(); ++it) {
                if (jm_zone_contains(&*it, msg[1], msg[2])) {
                  printf("sg num: %li\n", plugin->sampler.sound_gens_size());
                  // oops we hit polyphony, remove oldest sound gen in the queue to make room
                  if (plugin->sampler.sound_gens_size() >= POLYPHONY) {
                    printf("hit poly lim!\n");
                    plugin->sampler.sound_gens_tail()->sg->release_resources();
                    plugin->sampler.sound_gens_remove_last();
                  }

                  // shut off any sound gens that are in this off group
                  if (it->group > 0) {
                    for (sg_el = plugin->sampler.sound_gens_head(); sg_el != NULL; sg_el = sg_el->next) {
                      if (sg_el->sg->off_group == it->group)
                        sg_el->sg->set_release();
                    }
                  }

                  // create sound gen
                  AmpEnvGenerator* ag = plugin->sampler.new_amp_gen();
                  Playhead* ph = plugin->sampler.new_playhead();
                  ph->init(*it, msg[1]);
                  ag->init(ph, *it, msg[1], msg[2]);
                  printf("pre process start\n");
                  ag->pre_process(n_samples - n);
                  printf("pre process finish\n");

                  // add sound gen to queue
                  plugin->sampler.sound_gens_add(ag);
                  printf("event: channel: %i; note on;  note: %i; vel: %i\n", msg[0] & 0x0F, msg[1], msg[2]);
                }
              }
            }
            // process note off
            else if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_NOTE_OFF) {
              printf("event: note off; note: %i\n", msg[1]);
              // find all sound gens assigned to this pitch
              for (sg_el = plugin->sampler.sound_gens_head(); sg_el != NULL; sg_el = sg_el->next) {
                if (sg_el->sg->pitch == msg[1]) {
                  // note off does not apply to one shot
                  if (!sg_el->sg->one_shot) {
                    // if sustaining, just mark for removal later
                    if (plugin->sampler.sustain_on) {
                      sg_el->sg->note_off = true;
                    }
                    // not sustaining, remove immediately
                    else
                      sg_el->sg->set_release();
                  }
                }
              }
            }
            // process sustain pedal
            else if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_CONTROLLER && msg[1] == LV2_MIDI_CTL_SUSTAIN) {
              // >= 64 turns on sustain
              if (msg[2] >= 64) {
                plugin->sampler.sustain_on = true;
                printf("sustain on\n");
              }
              // < 64 turns ustain off
              else {
                for (sg_el = plugin->sampler.sound_gens_head(); sg_el != NULL; sg_el = sg_el->next) {
                  // turn off all sound gens marked with previous note off
                  if (sg_el->sg->note_off == true)
                    sg_el->sg->set_release();
                }

                plugin->sampler.sustain_on = false;
                printf("sustain off\n");
              }
            }
            // just print messages we don't currently handle
            else if (lv2_midi_message_type(msg) != LV2_MIDI_MSG_ACTIVE_SENSE)
              printf("event: 0x%x\n", msg[0]);
          }
        }
        // get next midi event or break if none left
        ev = lv2_atom_sequence_next(ev);
        if (lv2_atom_sequence_is_end(&plugin->control_port->body, plugin->control_port->atom.size, ev))
          break;
      }
    }
    // loop sound gens and fill audio buffer at current time (frame) position
    for (sg_el = plugin->sampler.sound_gens_head(); sg_el != NULL; sg_el = sg_el->next) {
      float values[2];
      sg_el->sg->get_values(values);
      plugin->out1[n] += plugin->sampler.amp[plugin->sampler.level] * values[0];
      plugin->out2[n] += plugin->sampler.amp[plugin->sampler.level] * values[1];

      sg_el->sg->inc();
      if (sg_el->sg->is_finished()) {
        sg_el->sg->release_resources();
        plugin->sampler.sound_gens_remove(sg_el);
      }
    }
  }

  lv2_atom_forge_pop(&plugin->forge, &seq_frame);
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
