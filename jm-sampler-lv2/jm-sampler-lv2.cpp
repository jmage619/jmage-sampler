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

#include <lib/lv2_uris.h>
#include <lib/zone.h>
#include <lib/wave.h>
#include <lib/sfzparser.h>
#include <lib/jmsampler.h>

#define SAMPLE_RATE 44100
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
  union {
    char str[256];
    int i;
  } data;
};

struct jm_sampler_plugin {
  const LV2_Atom_Sequence* control_port;
  LV2_Atom_Sequence* notify_port;
  float* out1;
  float* out2;
  LV2_URID_Map* map;
  jm_uris uris;
  LV2_Worker_Schedule* schedule;
  LV2_Atom_Forge forge;
  LV2_Atom_Forge_Frame seq_frame;
  int zone_number; // only for naming
  sfz::sfz* patch;
  std::vector<jm_zone> zones;
  std::map<std::string, jm::wave> waves;
  char patch_path[256];
  float* channel;
  float* level;
  JMSampler* sampler;
};

static inline float get_amp(int index) {
  return index == 0 ? 0.f: 1 / 100.f * pow(10.f, 2 * index / (VOL_STEPS - 1.0f));
}

static LV2_Handle instantiate(const LV2_Descriptor*, double, const char*,
    const LV2_Feature* const* features) {
  jm_sampler_plugin* plugin = new jm_sampler_plugin;

  // Scan host features for URID map
  LV2_URID_Map* map = NULL;
  LV2_Worker_Schedule* schedule = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map))
      map = static_cast<LV2_URID_Map*>(features[i]->data);
    else if (!strcmp(features[i]->URI, LV2_WORKER__schedule))
      schedule = static_cast<LV2_Worker_Schedule*>(features[i]->data);
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

  plugin->schedule = schedule;

  // Map URIS
  plugin->map = map;
  jm_map_uris(plugin->map, &plugin->uris);
  lv2_atom_forge_init(&plugin->forge, plugin->map);

  plugin->zone_number = 1;
  plugin->patch = NULL;
  plugin->patch_path[0] = '\0';

  // pre-allocate vector to prevent allocations later in RT thread
  plugin->zones.reserve(100);

  plugin->sampler = new JMSampler(plugin->zones);

  fprintf(stderr, "sampler instantiated.\n");
  return plugin;
}

static void cleanup(LV2_Handle instance) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

  delete plugin->sampler;

  if (plugin->patch != NULL)
    delete plugin->patch;

  std::map<std::string, jm::wave>::iterator it;
  for (it = plugin->waves.begin(); it != plugin->waves.end(); ++it)
    jm::free_wave(it->second);

  delete plugin;
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);
  switch (port) {
    case SAMPLER_CONTROL:
      plugin->control_port = static_cast<const LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_VOLUME:
      plugin->level = (float*) data;
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

static void send_add_zone(jm_sampler_plugin* plugin, const jm_zone& zone) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_addZone);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  LV2_Atom_Forge_Frame tuple_frame;
  lv2_atom_forge_tuple(&plugin->forge, &tuple_frame);
  lv2_atom_forge_int(&plugin->forge, zone.wave_length);
  lv2_atom_forge_string(&plugin->forge, zone.name, strlen(zone.name));
  lv2_atom_forge_float(&plugin->forge, zone.amp);
  lv2_atom_forge_int(&plugin->forge, zone.origin);
  lv2_atom_forge_int(&plugin->forge, zone.low_key);
  lv2_atom_forge_int(&plugin->forge, zone.high_key);
  lv2_atom_forge_int(&plugin->forge, zone.low_vel);
  lv2_atom_forge_int(&plugin->forge, zone.high_vel);
  lv2_atom_forge_double(&plugin->forge, zone.pitch_corr);
  lv2_atom_forge_int(&plugin->forge, zone.start);
  lv2_atom_forge_int(&plugin->forge, zone.left);
  lv2_atom_forge_int(&plugin->forge, zone.right);
  lv2_atom_forge_int(&plugin->forge, zone.loop_mode);
  lv2_atom_forge_int(&plugin->forge, zone.crossfade);
  lv2_atom_forge_int(&plugin->forge, zone.group);
  lv2_atom_forge_int(&plugin->forge, zone.off_group);
  lv2_atom_forge_int(&plugin->forge, zone.attack);
  lv2_atom_forge_int(&plugin->forge, zone.hold);
  lv2_atom_forge_int(&plugin->forge, zone.decay);
  lv2_atom_forge_float(&plugin->forge, zone.sustain);
  lv2_atom_forge_int(&plugin->forge, zone.release);
  lv2_atom_forge_string(&plugin->forge, zone.path, strlen(zone.path));
  lv2_atom_forge_pop(&plugin->forge, &tuple_frame);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: add zone sent!! %s\n", zone.name);
}

static void send_remove_zone(jm_sampler_plugin* plugin, int index) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_removeZone);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_int(&plugin->forge, index);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: remove zone sent!! %i\n", index);
}

static void send_update_vol(jm_sampler_plugin* plugin, float vol) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_updateVol);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_float(&plugin->forge, vol);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update vol sent!! %f\n", vol);
}

static void send_update_chan(jm_sampler_plugin* plugin, float chan) {
  lv2_atom_forge_frame_time(&plugin->forge, 0);
  LV2_Atom_Forge_Frame obj_frame;
  lv2_atom_forge_object(&plugin->forge, &obj_frame, 0, plugin->uris.jm_updateChan);
  lv2_atom_forge_key(&plugin->forge, plugin->uris.jm_params);
  lv2_atom_forge_float(&plugin->forge, chan);
  lv2_atom_forge_pop(&plugin->forge, &obj_frame);

  fprintf(stderr, "SAMPLER: update chan sent!! %f\n", chan);
}

static void update_zone(jm_sampler_plugin* plugin, const LV2_Atom_Object* obj) {
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
      plugin->zones.at(index).loop_mode = (jm_loop_mode) reinterpret_cast<LV2_Atom_Int*>(a)->body;
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

static void add_zone_from_wave(jm_sampler_plugin* plugin, const char* path) {
  jm::wave& wav = plugin->waves[path];
  jm_zone zone;
  jm_init_zone(&zone);
  zone.wave = wav.wave;
  zone.num_channels = wav.num_channels;
  zone.wave_length = wav.length;
  zone.left = wav.left;
  zone.right = wav.length;
  if (wav.has_loop)
    zone.loop_mode = LOOP_CONTINUOUS;
  sprintf(zone.name, "Zone %i", plugin->zone_number++);
  strcpy(zone.path, path);
  plugin->zones.push_back(zone);

  send_add_zone(plugin, zone);
}

static void add_zone_from_region(jm_sampler_plugin* plugin, const std::map<std::string, SFZValue>& region) {
  jm::wave& wav = plugin->waves[region.find("sample")->second.get_str()];
  jm_zone zone;
  jm_init_zone(&zone);
  zone.wave = wav.wave;
  zone.num_channels = wav.num_channels;
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

  jm_loop_mode mode = (jm_loop_mode) region.find("loop_mode")->second.get_int();
  if (mode != LOOP_UNSET)
    zone.loop_mode = mode;

  int loop_start = region.find("loop_start")->second.get_int();
  if (loop_start >= 0)
    zone.left = loop_start;

  int loop_end = region.find("loop_end")->second.get_int();
  if (loop_end >= 0)
    zone.right = loop_end;

  zone.crossfade = SAMPLE_RATE * region.find("loop_crossfade")->second.get_double();
  zone.group = region.find("group")->second.get_int();
  zone.off_group = region.find("off_group")->second.get_int();
  zone.attack = SAMPLE_RATE * region.find("ampeg_attack")->second.get_double();
  zone.hold = SAMPLE_RATE * region.find("ampeg_hold")->second.get_double();
  zone.decay = SAMPLE_RATE * region.find("ampeg_decay")->second.get_double();
  zone.sustain = region.find("ampeg_sustain")->second.get_double() / 100.;
  zone.release = SAMPLE_RATE * region.find("ampeg_release")->second.get_double();
  plugin->zones.push_back(zone);

  send_add_zone(plugin, zone);
}

static LV2_Worker_Status work(LV2_Handle instance, LV2_Worker_Respond_Function respond,
    LV2_Worker_Respond_Handle handle, uint32_t, const void* data) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

  const worker_msg* msg = static_cast<const worker_msg*>(data);
  if (msg->type == WORKER_LOAD_PATH_WAV) {
    plugin->waves[msg->data.str] = jm::parse_wave(msg->data.str);

    respond(handle, sizeof(worker_msg), msg); 
    //fprintf(stderr, "SAMPLER: work completed; parsed: %s\n", path);
  }
  else if (msg->type == WORKER_LOAD_REGION_WAV) {
    std::map<std::string, SFZValue>& reg = plugin->patch->regions.at(msg->data.i);

    std::string wav_path = reg["sample"].get_str();
    
    plugin->waves[wav_path] = jm::parse_wave(wav_path.c_str());

    respond(handle, sizeof(worker_msg), msg); 
  }
  else if (msg->type == WORKER_LOAD_PATCH) {
    SFZParser* parser;
    int len = strlen(msg->data.str);
    if (!strcmp(msg->data.str + len - 4, ".jmz"))
      parser = new JMZParser(msg->data.str);
    // assumed it could ony eitehr be jmz or sfz
    else
      parser = new SFZParser(msg->data.str);

    fprintf(stderr, "SAMPLER: work loading patch: %s\n", msg->data.str);
    if (plugin->patch != NULL)
      delete plugin->patch;

    plugin->patch = parser->parse();
    delete parser;

    strcpy(plugin->patch_path, msg->data.str);

    respond(handle, sizeof(worker_msg), msg); 
  }
  else if (msg->type == WORKER_SAVE_PATCH) {
    int len = strlen(msg->data.str);

    sfz::sfz save_patch;

    bool is_jmz = !strcmp(msg->data.str + len - 4, ".jmz");
    if (is_jmz) {
      save_patch.control["jm_vol"] = (int) *plugin->level;
      save_patch.control["jm_chan"] = (int) *plugin->channel + 1;
    }

    std::vector<jm_zone>::iterator it;
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
      region["loop_crossfade"] = (double) it->crossfade / SAMPLE_RATE;
      region["group"] = it->group;
      region["off_group"] = it->off_group;
      region["ampeg_attack"] = (double) it->attack / SAMPLE_RATE;
      region["ampeg_hold"] = (double) it->hold / SAMPLE_RATE;
      region["ampeg_decay"] = (double) it->decay / SAMPLE_RATE;
      region["ampeg_sustain"] = 100. * it->sustain;
      region["ampeg_release"] = (double) it->release / SAMPLE_RATE;

      save_patch.regions.push_back(region);
    }

    std::ofstream fout(msg->data.str);

    sfz::write(&save_patch, fout);
    fout.close();

    // probably should notify UI here that we finished!
  }

  return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status work_response(LV2_Handle instance, uint32_t, const void* data) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);
  const worker_msg* msg = static_cast<const worker_msg*>(data);

  if (msg->type == WORKER_LOAD_PATH_WAV) {
    //LV2_Atom_Forge_Frame seq_frame;
    //lv2_atom_forge_sequence_head(&plugin->forge, &seq_frame, 0);
    add_zone_from_wave(plugin, msg->data.str);
    //lv2_atom_forge_pop(&plugin->forge, &seq_frame);
    //fprintf(stderr, "SAMPLER: response completed; added: %s\n", path);
  }
  else if (msg->type == WORKER_LOAD_REGION_WAV) {
    fprintf(stderr, "SAMPLER load region wav response!!\n");
    std::map<std::string, SFZValue>& reg = plugin->patch->regions.at(msg->data.i);
    add_zone_from_region(plugin, reg);
  }
  else if (msg->type == WORKER_LOAD_PATCH) {
    fprintf(stderr, "SAMPLER load patch response!! num regions: %i\n", (int) plugin->patch->regions.size());

    int num_zones = (int) plugin->zones.size();

    for (int i = 0; i < num_zones; ++i)
      send_remove_zone(plugin, 0);

    plugin->zone_number = 1;

    std::map<std::string, SFZValue>::iterator c_it = plugin->patch->control.find("jm_vol");
    if (c_it != plugin->patch->control.end()) {
      send_update_vol(plugin, c_it->second.get_int());
      send_update_chan(plugin, plugin->patch->control["jm_chan"].get_int() - 1);
    }
    // reset to reasonable defaults if not defined
    else {
      send_update_vol(plugin, 16);
      send_update_chan(plugin, 0);
    }

    plugin->zones.erase(plugin->zones.begin(), plugin->zones.end());
    std::vector<std::map<std::string, SFZValue>>::iterator it;
    for (it = plugin->patch->regions.begin(); it != plugin->patch->regions.end(); ++it) {
      if (plugin->waves.find((*it)["sample"].get_str()) == plugin->waves.end()) {
        worker_msg reg_msg;
        reg_msg.type = WORKER_LOAD_REGION_WAV;
        reg_msg.data.i = it - plugin->patch->regions.begin();
        plugin->schedule->schedule_work(plugin->schedule->handle, sizeof(worker_msg), &reg_msg);
      }
      else
        add_zone_from_region(plugin, *it);
    }
  }

  return LV2_WORKER_SUCCESS;
}

// later most of this opaque logic should be moved to member funs
// consider everything in common w/ stand alone jack audio callback when we re-implement that version
static void run(LV2_Handle instance, uint32_t n_samples) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

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
    if (ev->body.type == plugin->uris.atom_Object) {
      const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
      if (obj->body.otype == plugin->uris.jm_getZones) {
        //fprintf(stderr, "SAMPLER: get zones received!!\n");
        std::vector<jm_zone>::iterator it;
        for (it = plugin->zones.begin(); it != plugin->zones.end(); ++it) {
          send_add_zone(plugin, *it);
        }
      }
      else if (obj->body.otype == plugin->uris.jm_updateZone) {
        //fprintf(stderr, "SAMPLER: update zone received!!\n");
        update_zone(plugin, obj);
      }
      else if (obj->body.otype == plugin->uris.jm_addZone) {
        //fprintf(stderr, "SAMPLER: add zone received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
 
        if (plugin->waves.find(path) == plugin->waves.end()) {
          worker_msg msg;
          msg.type =  WORKER_LOAD_PATH_WAV;
          strcpy(msg.data.str, path);
          plugin->schedule->schedule_work(plugin->schedule->handle, sizeof(worker_msg), &msg);
        }
        else
          add_zone_from_wave(plugin, path);
      }
      else if (obj->body.otype == plugin->uris.jm_loadPatch) {
        //fprintf(stderr, "SAMPLER: load patch received!!\n");
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
        worker_msg msg;
        msg.type =  WORKER_LOAD_PATCH;
        strcpy(msg.data.str, path);
        plugin->schedule->schedule_work(plugin->schedule->handle, sizeof(worker_msg), &msg);
      }
      else if (obj->body.otype == plugin->uris.jm_savePatch) {
        LV2_Atom* params = NULL;

        lv2_atom_object_get(obj, plugin->uris.jm_params, &params, 0);
        char* path = (char*)(params + 1);
        worker_msg msg;
        msg.type =  WORKER_SAVE_PATCH;
        strcpy(msg.data.str, path);
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

    plugin->sampler->process_frame(n, get_amp(*plugin->level), plugin->out1, plugin->out2);
  }

  //lv2_atom_forge_pop(&plugin->forge, &seq_frame);
  //lv2_atom_forge_pop(&plugin->forge, &plugin->seq_frame);
}

static LV2_State_Status save(LV2_Handle instance, LV2_State_Store_Function store,
    LV2_State_Handle handle, uint32_t, const LV2_Feature* const* features) {
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

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
  jm_sampler_plugin* plugin = static_cast<jm_sampler_plugin*>(instance);

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
    add_zone_from_region(plugin, *it);
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
