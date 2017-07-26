#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <pthread.h>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>

#include <lib/lv2_uris.h>
#include <lib/zone.h>
#include <lib/wave.h>
#include <lib/sfzparser.h>
#include <lib/jmsampler.h>

#include "lv2sampler.h"

enum {
  SAMPLER_CONTROL = 0,
  SAMPLER_VOLUME = 1,
  SAMPLER_CHANNEL = 2,
  SAMPLER_NOTIFY  = 3,
  SAMPLER_OUT_L = 4,
  SAMPLER_OUT_R = 5
};

enum worker_msg_type {
  WORKER_LOAD_PATH_WAV,
  WORKER_LOAD_PATCH,
  WORKER_SAVE_PATCH
};

struct worker_msg {
  worker_msg_type type;
  int i;
};

static LV2_Handle instantiate(const LV2_Descriptor*, double sample_rate, const char*,
    const LV2_Feature* const* features) {

  // Scan host features for URID map
  LV2_URID_Map* map = NULL;
  LV2_Worker_Schedule* schedule = NULL;
  LV2_Options_Option* opt = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map))
      map = static_cast<LV2_URID_Map*>(features[i]->data);
    else if (!strcmp(features[i]->URI, LV2_WORKER__schedule))
      schedule = static_cast<LV2_Worker_Schedule*>(features[i]->data);
    else if (!strcmp(features[i]->URI, LV2_OPTIONS__options))
      opt = (LV2_Options_Option*)features[i]->data;
  }
  if (!map) {
    fprintf(stderr, "Host does not support urid:map.\n");
    return NULL;
  }
  if (!schedule) {
    fprintf(stderr, "Host does not support work:schedule.\n");
    return NULL;
  }
  if (!opt) {
    fprintf(stderr, "Host does not support opt:options.\n");
    return NULL;
  }

  int max_block_len = -1;
  int nominal_block_len = -1;

  // Map URIS
  jm::uris uris;
  jm::map_uris(map, &uris);

  int index = 0;
  while (1) {
    if (opt[index].key == 0)
      break;

    if (opt[index].key == uris.bufsize_maxBlockLength) {
      max_block_len = *((int*) opt[index].value);
      fprintf(stderr, "SAMPLER max block len: %i\n", max_block_len);
    }
    else if (opt[index].key == uris.bufsize_nominalBlockLength) {
      nominal_block_len = *((int*) opt[index].value);
      fprintf(stderr, "SAMPLER nominal block len: %i\n", nominal_block_len);
    }
    ++index;    
  }

  if (max_block_len < 0) {
    fprintf(stderr, "Host did not set bufsz:maxBlockLength.\n");
    return NULL;
  }

  if (nominal_block_len < 0)
    nominal_block_len = max_block_len;

  // src output buf just needs to hold up to nframes so max is sufficient
  // but set src input buf to nominal for efficiency
  LV2Sampler* sampler = new LV2Sampler(sample_rate, nominal_block_len, max_block_len);
  sampler->schedule = schedule;

  sampler->uris = uris;
  sampler->map = map;
  lv2_atom_forge_init(&sampler->forge, sampler->map);

  fprintf(stderr, "sampler instantiated.\n");
  return sampler;
}

static void cleanup(LV2_Handle instance) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);

  delete sampler;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);
  switch (port) {
    case SAMPLER_CONTROL:
      sampler->control_port = static_cast<const LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_VOLUME:
      sampler->volume = (float*) data;
      break;
    case SAMPLER_CHANNEL:
      sampler->channel = (float*) data;
      break;
    case SAMPLER_NOTIFY:
      sampler->notify_port = static_cast<LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_OUT_L:
      sampler->out1 = (float*) data;
      break;
    case SAMPLER_OUT_R:
      sampler->out2 = (float*) data;
      break;
    default:
      break;
  }
}

static LV2_Worker_Status work(LV2_Handle instance, LV2_Worker_Respond_Function respond,
    LV2_Worker_Respond_Handle handle, uint32_t, const void* data) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);

  const worker_msg* msg = static_cast<const worker_msg*>(data);
  if (msg->type == WORKER_LOAD_PATH_WAV) {
    sampler->waves[sampler->wav_path] = jm::parse_wave(sampler->wav_path);

    respond(handle, sizeof(worker_msg), msg); 
    //fprintf(stderr, "SAMPLER: work completed; parsed: %s\n", path);
  }
  else if (msg->type == WORKER_LOAD_PATCH) {
    fprintf(stderr, "SAMPLER: work loading patch: %s\n", sampler->patch_path);
    sampler->load_patch(sampler->patch_path);

    respond(handle, sizeof(worker_msg), msg); 
  }
  else if (msg->type == WORKER_SAVE_PATCH) {
    sampler->save_patch(sampler->patch_path);
    // probably should notify UI here that we finished!
  }

  return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status work_response(LV2_Handle instance, uint32_t, const void* data) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);
  const worker_msg* msg = static_cast<const worker_msg*>(data);

  if (msg->type == WORKER_LOAD_PATH_WAV) {
    //LV2_Atom_Forge_Frame seq_frame;
    //lv2_atom_forge_sequence_head(&plugin->forge, &seq_frame, 0);
    sampler->add_zone_from_wave(msg->i, sampler->wav_path);
    //lv2_atom_forge_pop(&plugin->forge, &seq_frame);
    //fprintf(stderr, "SAMPLER: response completed; added: %s\n", path);
  }
  else if (msg->type == WORKER_LOAD_PATCH) {
    fprintf(stderr, "SAMPLER load patch response!! num regions: %i\n", (int) sampler->patch.regions.size());

    sampler->send_clear_zones();

    std::map<std::string, SFZValue>::iterator c_it = sampler->patch.control.find("jm_vol");
    if (c_it != sampler->patch.control.end()) {
      sampler->send_update_vol(c_it->second.get_int());
      sampler->send_update_chan(sampler->patch.control["jm_chan"].get_int() - 1);
    }
    // reset to reasonable defaults if not defined
    else {
      sampler->send_update_vol(16);
      sampler->send_update_chan(0);
    }

    int num_zones = sampler->zones.size();

    for (int i = 0; i < num_zones; ++i)
      sampler->send_add_zone(i);
  }

  return LV2_WORKER_SUCCESS;
}

// later most of this opaque logic should be moved to member funs
// consider everything in common w/ stand alone jack audio callback when we re-implement that version
static void run(LV2_Handle instance, uint32_t n_samples) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);

  memset(sampler->out1, 0, sizeof(float) * n_samples);
  memset(sampler->out2, 0, sizeof(float) * n_samples);

  // Set up forge to write directly to notify output port.
  const uint32_t notify_capacity = sampler->notify_port->atom.size;
  lv2_atom_forge_set_buffer(&sampler->forge, reinterpret_cast<uint8_t*>(sampler->notify_port),
    notify_capacity);

  //LV2_Atom_Forge_Frame seq_frame;
  //lv2_atom_forge_sequence_head(&sampler->forge, &seq_frame, 0);
  lv2_atom_forge_sequence_head(&sampler->forge, &sampler->seq_frame, 0);

  // we used to handle ui messages here
  // but now they will be in the same atom list as midi

  // must loop all atoms to find UI messages; not sure if qtractor bug
  // but they all occur at frame == n_samples (outside the range!)
  LV2_ATOM_SEQUENCE_FOREACH(sampler->control_port, ev) {
    if (lv2_atom_forge_is_object_type(&sampler->forge, ev->body.type)) {
      const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
      if (obj->body.otype == sampler->uris.jm_getSampleRate) {
        sampler->send_sample_rate();
      }
      else if (obj->body.otype == sampler->uris.jm_getZoneVect) {
        sampler->send_zone_vect();
      }
      else if (obj->body.otype == sampler->uris.jm_updateZone) {
        //fprintf(stderr, "SAMPLER: update zone received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, sampler->uris.jm_params, &params, 0);
        LV2_Atom* a = lv2_atom_tuple_begin((LV2_Atom_Tuple*) params);
        int index = reinterpret_cast<LV2_Atom_Int*>(a)->body;
        a = lv2_atom_tuple_next(a);
        int key = reinterpret_cast<LV2_Atom_Int*>(a)->body;
        a = lv2_atom_tuple_next(a);
        sampler->update_zone(index, key, (char*)(a + 1));
      }
      else if (obj->body.otype == sampler->uris.jm_removeZone) {
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, sampler->uris.jm_params, &params, 0);
        int index = ((LV2_Atom_Int*) params)->body;
        //fprintf(stderr, "SAMPLER: remove zone received!! index: %i\n", index);
        sampler->remove_zone(index);
        sampler->send_remove_zone(index);
      }
      else if (obj->body.otype == sampler->uris.jm_addZone) {
        //fprintf(stderr, "SAMPLER: add zone received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, sampler->uris.jm_params, &params, 0);
        LV2_Atom* a = lv2_atom_tuple_begin((LV2_Atom_Tuple*) params);
        int index = ((LV2_Atom_Int*) a)->body;
        a = lv2_atom_tuple_next(a);
        char* path = (char*)(a + 1);
 
        if (sampler->waves.find(path) == sampler->waves.end()) {
          worker_msg msg;
          msg.type =  WORKER_LOAD_PATH_WAV;
          msg.i = index;
          strcpy(sampler->wav_path, path);
          sampler->schedule->schedule_work(sampler->schedule->handle, sizeof(worker_msg), &msg);
        }
        else
          sampler->add_zone_from_wave(index, path);
      }
      else if (obj->body.otype == sampler->uris.jm_loadPatch) {
        //fprintf(stderr, "SAMPLER: load patch received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, sampler->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
        worker_msg msg;
        msg.type =  WORKER_LOAD_PATCH;
        strcpy(sampler->patch_path, path);
        sampler->schedule->schedule_work(sampler->schedule->handle, sizeof(worker_msg), &msg);
      }
      else if (obj->body.otype == sampler->uris.jm_savePatch) {
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, sampler->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
        worker_msg msg;
        msg.type =  WORKER_SAVE_PATCH;
        strcpy(sampler->patch_path, path);
        sampler->schedule->schedule_work(sampler->schedule->handle, sizeof(worker_msg), &msg);
      }
    }
  }

  sampler->pre_process(n_samples);

  LV2_Atom_Event* ev = lv2_atom_sequence_begin(&sampler->control_port->body);

  // loop over frames in this callback window
  for (uint32_t n = 0; n < n_samples; ++n) {
    if (!lv2_atom_sequence_is_end(&sampler->control_port->body, sampler->control_port->atom.size, ev)) {
      // procces next event if it applies to current time (frame)
      while (n == ev->time.frames) {
        if (ev->body.type == sampler->uris.midi_Event) {
          const uint8_t* const msg = (const uint8_t*)(ev + 1);
          // only consider events from current channel
          if ((msg[0] & 0x0f) == (int) *sampler->channel) {
            // process note on
            if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_NOTE_ON) {
              sampler->handle_note_on(msg, n_samples, n);
            }
            // process note off
            else if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_NOTE_OFF)
              sampler->handle_note_off(msg);
            // process sustain pedal
            else if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_CONTROLLER && msg[1] == LV2_MIDI_CTL_SUSTAIN)
              sampler->handle_sustain(msg);
            // just print messages we don't currently handle
            else if (lv2_midi_message_type(msg) != LV2_MIDI_MSG_ACTIVE_SENSE)
              fprintf(stderr, "event: 0x%x\n", msg[0]);
          }
        }
        // get next midi event or break if none left
        ev = lv2_atom_sequence_next(ev);
        if (lv2_atom_sequence_is_end(&sampler->control_port->body, sampler->control_port->atom.size, ev))
          break;
      }
    }

    sampler->process_frame(n, sampler->out1, sampler->out2);
  }

  //lv2_atom_forge_pop(&sampler->forge, &seq_frame);
  //lv2_atom_forge_pop(&sampler->forge, &sampler->seq_frame);
}

static LV2_State_Status save(LV2_Handle instance, LV2_State_Store_Function store,
    LV2_State_Handle handle, uint32_t, const LV2_Feature* const* features) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);

  if (sampler->patch_path[0] == '\0')
    return LV2_STATE_SUCCESS;

  LV2_State_Map_Path* map_path = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_STATE__mapPath))
      map_path = static_cast<LV2_State_Map_Path*>(features[i]->data);
  }

  if (map_path == NULL)
    return LV2_STATE_ERR_NO_FEATURE;

  char* apath = map_path->abstract_path(map_path->handle, sampler->patch_path);

  store(handle, sampler->uris.jm_patchFile, apath, strlen(apath) + 1,
    sampler->uris.atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  delete apath;

  return LV2_STATE_SUCCESS;
}

static LV2_State_Status restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve,
    LV2_State_Handle handle, uint32_t, const LV2_Feature* const* features) {
  LV2Sampler* sampler = static_cast<LV2Sampler*>(instance);

  size_t   size;
  uint32_t type;
  uint32_t valflags;
  const void* value = retrieve(handle, sampler->uris.jm_patchFile, &size, &type, &valflags);

  if (value == NULL)
    return LV2_STATE_SUCCESS;
    
  LV2_State_Map_Path* map_path = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_STATE__mapPath))
      map_path = static_cast<LV2_State_Map_Path*>(features[i]->data);
  }

  if (map_path == NULL)
    return LV2_STATE_ERR_NO_FEATURE;

  const char* apath = static_cast<const char*>(value);
  char* path = map_path->absolute_path(map_path->handle, apath);

  strcpy(sampler->patch_path, path);
  delete path;

  sampler->load_patch(sampler->patch_path);

  return LV2_STATE_SUCCESS;
}

static const void* extension_data(const char* uri) {
  static const LV2_Worker_Interface worker = { work, work_response, NULL };
  static const LV2_State_Interface state = { save, restore };
  if (!strcmp(uri, LV2_WORKER__interface))
    return &worker;
  if (!strcmp(uri, LV2_STATE__interface))
    return &state;

  return NULL;
}

static const LV2_Descriptor descriptor = {
  JM_SAMPLER_URI,
  instantiate,
  connect_port,
  NULL, // activate
  run,
  NULL, // deactivate
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index) {
  switch (index) {
    case 0:
      return &descriptor;
    default:
      return NULL;
  }
}
