#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <stdexcept>

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

#include "plugin.h"

#define VOL_STEPS 17

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
  WORKER_LOAD_REGION_WAV,
  WORKER_LOAD_PATCH,
  WORKER_SAVE_PATCH
};

struct worker_msg {
  worker_msg_type type;
  int i;
};

static inline float get_amp(int index) {
  return index == 0 ? 0.f: 1 / 100.f * pow(10.f, 2 * index / (VOL_STEPS - 1.0f));
}

static LV2_Handle instantiate(const LV2_Descriptor*, double sample_rate, const char*,
    const LV2_Feature* const* features) {
  jm::sampler_plugin* plugin = new jm::sampler_plugin;
  plugin->sample_rate = sample_rate;

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
    delete plugin;
    return NULL;
  }
  if (!schedule) {
    fprintf(stderr, "Host does not support work:schedule.\n");
    delete plugin;
    return NULL;
  }
  if (!opt) {
    fprintf(stderr, "Host does not support opt:options.\n");
    delete plugin;
    return NULL;
  }

  plugin->schedule = schedule;

  // Map URIS
  plugin->map = map;
  jm::map_uris(plugin->map, &plugin->uris);
  lv2_atom_forge_init(&plugin->forge, plugin->map);

  int max_block_len = -1;
  int nominal_block_len = -1;

  int index = 0;
  while (1) {
    if (opt[index].key == 0)
      break;

    if (opt[index].key == plugin->uris.bufsize_maxBlockLength) {
      max_block_len = *((int*) opt[index].value);
      fprintf(stderr, "SAMPLER max block len: %i\n", max_block_len);
    }
    else if (opt[index].key == plugin->uris.bufsize_nominalBlockLength) {
      nominal_block_len = *((int*) opt[index].value);
      fprintf(stderr, "SAMPLER nominal block len: %i\n", nominal_block_len);
    }
    ++index;    
  }

  if (max_block_len < 0) {
    fprintf(stderr, "Host did not set bufsz:maxBlockLength.\n");
    delete plugin;
    return NULL;
  }

  if (nominal_block_len < 0)
    nominal_block_len = max_block_len;

  plugin->zone_number = 1;
  plugin->patch = NULL;
  plugin->patch_path[0] = '\0';

  // pre-allocate vector to prevent allocations later in RT thread
  plugin->zones.reserve(100);

  // src output buf just needs to hold up to nframes so max is sufficient
  // but set src input buf to nominal for efficiency
  plugin->sampler = new JMSampler(plugin->zones, sample_rate, nominal_block_len, max_block_len);

  fprintf(stderr, "sampler instantiated.\n");
  return plugin;
}

static void cleanup(LV2_Handle instance) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);

  delete plugin->sampler;

  if (plugin->patch != NULL)
    delete plugin->patch;

  std::map<std::string, jm::wave>::iterator it;
  for (it = plugin->waves.begin(); it != plugin->waves.end(); ++it)
    jm::free_wave(it->second);

  delete plugin;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);
  switch (port) {
    case SAMPLER_CONTROL:
      plugin->control_port = static_cast<const LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_VOLUME:
      plugin->volume = (float*) data;
      break;
    case SAMPLER_CHANNEL:
      plugin->channel = (float*) data;
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

static LV2_Worker_Status work(LV2_Handle instance, LV2_Worker_Respond_Function respond,
    LV2_Worker_Respond_Handle handle, uint32_t, const void* data) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);

  const worker_msg* msg = static_cast<const worker_msg*>(data);
  if (msg->type == WORKER_LOAD_PATH_WAV) {
    plugin->waves[plugin->wav_path] = jm::parse_wave(plugin->wav_path);

    respond(handle, sizeof(worker_msg), msg); 
    //fprintf(stderr, "SAMPLER: work completed; parsed: %s\n", path);
  }
  else if (msg->type == WORKER_LOAD_REGION_WAV) {
    std::map<std::string, SFZValue>& reg = plugin->patch->regions.at(msg->i);

    std::string wav_path = reg["sample"].get_str();
    
    plugin->waves[wav_path] = jm::parse_wave(wav_path.c_str());

    respond(handle, sizeof(worker_msg), msg); 
  }
  else if (msg->type == WORKER_LOAD_PATCH) {
    SFZParser* parser;
    int len = strlen(plugin->patch_path);
    if (!strcmp(plugin->patch_path + len - 4, ".jmz"))
      parser = new JMZParser(plugin->patch_path);
    // assumed it could ony eitehr be jmz or sfz
    else
      parser = new SFZParser(plugin->patch_path);

    fprintf(stderr, "SAMPLER: work loading patch: %s\n", plugin->patch_path);
    if (plugin->patch != NULL)
      delete plugin->patch;

    plugin->patch = parser->parse();
    delete parser;

    respond(handle, sizeof(worker_msg), msg); 
  }
  else if (msg->type == WORKER_SAVE_PATCH) {
    int len = strlen(plugin->patch_path);

    sfz::sfz save_patch;

    bool is_jmz = !strcmp(plugin->patch_path + len - 4, ".jmz");
    if (is_jmz) {
      save_patch.control["jm_vol"] = (int) *plugin->volume;
      save_patch.control["jm_chan"] = (int) *plugin->channel + 1;
    }

    std::vector<jm::zone>::iterator it;
    for (it = plugin->zones.begin(); it != plugin->zones.end(); ++it) {
      std::map<std::string, SFZValue> region;

      if (is_jmz)
        region["jm_name"] = it->name;

      region["sample"] = it->path;
      region["volume"] = 20. * log10(it->amp);
      region["lokey"] = it->low_key;
      region["hikey"] = it->high_key;
      region["pitch_keycenter"] = it->origin;
      region["lovel"] = it->low_vel;
      region["hivel"] = it->high_vel;
      region["tune"] = (int) (100. * it->pitch_corr);
      region["offset"] = it->start;
      region["loop_mode"] = it->loop_mode;
      region["loop_start"] = it->left;
      region["loop_end"] = it->right;
      region["loop_crossfade"] = (double) it->crossfade / plugin->sample_rate;
      region["group"] = it->group;
      region["off_group"] = it->off_group;
      region["ampeg_attack"] = (double) it->attack / plugin->sample_rate;
      region["ampeg_hold"] = (double) it->hold / plugin->sample_rate;
      region["ampeg_decay"] = (double) it->decay / plugin->sample_rate;
      region["ampeg_sustain"] = 100. * it->sustain;
      region["ampeg_release"] = (double) it->release / plugin->sample_rate;

      save_patch.regions.push_back(region);
    }

    std::ofstream fout(plugin->patch_path);

    sfz::write(&save_patch, fout);
    fout.close();

    // probably should notify UI here that we finished!
  }

  return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status work_response(LV2_Handle instance, uint32_t, const void* data) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);
  const worker_msg* msg = static_cast<const worker_msg*>(data);

  if (msg->type == WORKER_LOAD_PATH_WAV) {
    //LV2_Atom_Forge_Frame seq_frame;
    //lv2_atom_forge_sequence_head(&plugin->forge, &seq_frame, 0);
    jm::add_zone_from_wave(plugin, msg->i, plugin->wav_path);
    //lv2_atom_forge_pop(&plugin->forge, &seq_frame);
    //fprintf(stderr, "SAMPLER: response completed; added: %s\n", path);
  }
  else if (msg->type == WORKER_LOAD_REGION_WAV) {
    fprintf(stderr, "SAMPLER load region wav response!!\n");
    std::map<std::string, SFZValue>& reg = plugin->patch->regions.at(msg->i);
    jm::add_zone_from_region(plugin, reg);
  }
  else if (msg->type == WORKER_LOAD_PATCH) {
    fprintf(stderr, "SAMPLER load patch response!! num regions: %i\n", (int) plugin->patch->regions.size());

    int num_zones = (int) plugin->zones.size();

    for (int i = 0; i < num_zones; ++i)
      jm::send_remove_zone(plugin, 0);

    plugin->zone_number = 1;

    std::map<std::string, SFZValue>::iterator c_it = plugin->patch->control.find("jm_vol");
    if (c_it != plugin->patch->control.end()) {
      jm::send_update_vol(plugin, c_it->second.get_int());
      jm::send_update_chan(plugin, plugin->patch->control["jm_chan"].get_int() - 1);
    }
    // reset to reasonable defaults if not defined
    else {
      jm::send_update_vol(plugin, 16);
      jm::send_update_chan(plugin, 0);
    }

    plugin->zones.erase(plugin->zones.begin(), plugin->zones.end());
    std::vector<std::map<std::string, SFZValue>>::iterator it;
    for (it = plugin->patch->regions.begin(); it != plugin->patch->regions.end(); ++it) {
      std::string wav_path = (*it)["sample"].get_str();
      if (plugin->waves.find(wav_path) == plugin->waves.end()) {
        plugin->waves[wav_path] = jm::parse_wave(wav_path.c_str());
      }
      jm::add_zone_from_region(plugin, *it);
    }
  }

  return LV2_WORKER_SUCCESS;
}

// later most of this opaque logic should be moved to member funs
// consider everything in common w/ stand alone jack audio callback when we re-implement that version
static void run(LV2_Handle instance, uint32_t n_samples) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);

  memset(plugin->out1, 0, sizeof(float) * n_samples);
  memset(plugin->out2, 0, sizeof(float) * n_samples);

  // Set up forge to write directly to notify output port.
  const uint32_t notify_capacity = plugin->notify_port->atom.size;
  lv2_atom_forge_set_buffer(&plugin->forge, reinterpret_cast<uint8_t*>(plugin->notify_port),
    notify_capacity);

  //LV2_Atom_Forge_Frame seq_frame;
  //lv2_atom_forge_sequence_head(&plugin->forge, &seq_frame, 0);
  lv2_atom_forge_sequence_head(&plugin->forge, &plugin->seq_frame, 0);

  // we used to handle ui messages here
  // but now they will be in the same atom list as midi

  // must loop all atoms to find UI messages; not sure if qtractor bug
  // but they all occur at frame == n_samples (outside the range!)
  LV2_ATOM_SEQUENCE_FOREACH(plugin->control_port, ev) {
    if (lv2_atom_forge_is_object_type(&plugin->forge, ev->body.type)) {
      const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
      if (obj->body.otype == plugin->uris.jm_getSampleRate) {
        jm::send_sample_rate(plugin);
      }
      else if (obj->body.otype == plugin->uris.jm_getZoneVect) {
        jm::send_zone_vect(plugin);
      }
      else if (obj->body.otype == plugin->uris.jm_updateZone) {
        //fprintf(stderr, "SAMPLER: update zone received!!\n");
        jm::update_zone(plugin, obj);
      }
      else if (obj->body.otype == plugin->uris.jm_removeZone) {
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        int index = ((LV2_Atom_Int*) params)->body;
        //fprintf(stderr, "SAMPLER: remove zone received!! index: %i\n", index);
        plugin->zones.erase(plugin->zones.begin() + index);
        jm::send_remove_zone(plugin, index);
      }
      else if (obj->body.otype == plugin->uris.jm_addZone) {
        //fprintf(stderr, "SAMPLER: add zone received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        LV2_Atom* a = lv2_atom_tuple_begin((LV2_Atom_Tuple*) params);
        int index = ((LV2_Atom_Int*) a)->body;
        a = lv2_atom_tuple_next(a);
        char* path = (char*)(a + 1);
 
        if (plugin->waves.find(path) == plugin->waves.end()) {
          worker_msg msg;
          msg.type =  WORKER_LOAD_PATH_WAV;
          msg.i = index;
          strcpy(plugin->wav_path, path);
          plugin->schedule->schedule_work(plugin->schedule->handle, sizeof(worker_msg), &msg);
        }
        else
          jm::add_zone_from_wave(plugin, index, path);
      }
      else if (obj->body.otype == plugin->uris.jm_loadPatch) {
        //fprintf(stderr, "SAMPLER: load patch received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
        worker_msg msg;
        msg.type =  WORKER_LOAD_PATCH;
        strcpy(plugin->patch_path, path);
        plugin->schedule->schedule_work(plugin->schedule->handle, sizeof(worker_msg), &msg);
      }
      else if (obj->body.otype == plugin->uris.jm_savePatch) {
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
        worker_msg msg;
        msg.type =  WORKER_SAVE_PATCH;
        strcpy(plugin->patch_path, path);
        plugin->schedule->schedule_work(plugin->schedule->handle, sizeof(worker_msg), &msg);
      }
    }
  }

  plugin->sampler->pre_process(n_samples);

  LV2_Atom_Event* ev = lv2_atom_sequence_begin(&plugin->control_port->body);

  // loop over frames in this callback window
  for (uint32_t n = 0; n < n_samples; ++n) {
    if (!lv2_atom_sequence_is_end(&plugin->control_port->body, plugin->control_port->atom.size, ev)) {
      // procces next event if it applies to current time (frame)
      while (n == ev->time.frames) {
        if (ev->body.type == plugin->uris.midi_Event) {
          const uint8_t* const msg = (const uint8_t*)(ev + 1);
          // only consider events from current channel
          if ((msg[0] & 0x0f) == (int) *plugin->channel) {
            // process note on
            if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_NOTE_ON)
              plugin->sampler->handle_note_on(msg, n_samples, n);
            // process note off
            else if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_NOTE_OFF)
              plugin->sampler->handle_note_off(msg);
            // process sustain pedal
            else if (lv2_midi_message_type(msg) == LV2_MIDI_MSG_CONTROLLER && msg[1] == LV2_MIDI_CTL_SUSTAIN)
              plugin->sampler->handle_sustain(msg);
            // just print messages we don't currently handle
            else if (lv2_midi_message_type(msg) != LV2_MIDI_MSG_ACTIVE_SENSE)
              fprintf(stderr, "event: 0x%x\n", msg[0]);
          }
        }
        // get next midi event or break if none left
        ev = lv2_atom_sequence_next(ev);
        if (lv2_atom_sequence_is_end(&plugin->control_port->body, plugin->control_port->atom.size, ev))
          break;
      }
    }

    plugin->sampler->process_frame(n, get_amp(*plugin->volume), plugin->out1, plugin->out2);
  }

  //lv2_atom_forge_pop(&plugin->forge, &seq_frame);
  //lv2_atom_forge_pop(&plugin->forge, &plugin->seq_frame);
}

static LV2_State_Status save(LV2_Handle instance, LV2_State_Store_Function store,
    LV2_State_Handle handle, uint32_t, const LV2_Feature* const* features) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);

  if (plugin->patch_path[0] == '\0')
    return LV2_STATE_SUCCESS;

  LV2_State_Map_Path* map_path = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_STATE__mapPath))
      map_path = static_cast<LV2_State_Map_Path*>(features[i]->data);
  }

  if (map_path == NULL)
    return LV2_STATE_ERR_NO_FEATURE;

  char* apath = map_path->abstract_path(map_path->handle, plugin->patch_path);

  store(handle, plugin->uris.jm_patchFile, apath, strlen(apath) + 1,
    plugin->uris.atom_String, LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  delete apath;

  return LV2_STATE_SUCCESS;
}

static LV2_State_Status restore(LV2_Handle instance, LV2_State_Retrieve_Function retrieve,
    LV2_State_Handle handle, uint32_t, const LV2_Feature* const* features) {
  jm::sampler_plugin* plugin = static_cast<jm::sampler_plugin*>(instance);

  size_t   size;
  uint32_t type;
  uint32_t valflags;
  const void* value = retrieve(handle, plugin->uris.jm_patchFile, &size, &type, &valflags);

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

  // copied and pasted a lot from elsewhere.. consider wrapping in funs
  SFZParser* parser;
  int len = strlen(path);
  if (!strcmp(path + len - 4, ".jmz"))
    parser = new JMZParser(path);
  // assumed it could ony eitehr be jmz or sfz
  else
    parser = new SFZParser(path);

  fprintf(stderr, "SAMPLER: restore loading patch: %s\n", path);
  if (plugin->patch != NULL)
    delete plugin->patch;

  try {
    plugin->patch = parser->parse();
  }
  catch (std::runtime_error& e) {
    fprintf(stderr, "restore failed parsing patch: %s\n", e.what());
    delete parser;
    delete path;
    return LV2_STATE_ERR_UNKNOWN;
  }

  delete parser;

  strcpy(plugin->patch_path, path);
  delete path;

  // skipping vol and chan updates; should get those automatically from save state
  /*std::map<std::string, sfz::Value>::iterator c_it = plugin->patch->control.find("jm_vol");
  if (c_it != plugin->patch->control.end()) {
    send_update_vol(plugin, c_it->second.get_int());
    send_update_chan(plugin, plugin->patch->control["jm_chan"].get_int() - 1);
  }
  // reset to reasonable defaults if not defined
  else {
    send_update_vol(plugin, 16);
    send_update_chan(plugin, 0);
  }
  */

  std::vector<std::map<std::string, SFZValue>>::iterator it;
  for (it = plugin->patch->regions.begin(); it != plugin->patch->regions.end(); ++it) {
    if (plugin->waves.find((*it)["sample"].get_str()) == plugin->waves.end()) {
      std::string wav_path = (*it)["sample"].get_str();
      plugin->waves[wav_path] = jm::parse_wave(wav_path.c_str());
    }
    jm::add_zone_from_region(plugin, *it);
  }

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
