#include <cstdlib>
#include <cstddef>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cmath>

#include <iostream>
#include <string>
#include <vector>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
//#include <lv2/lv2plug.in/ns/ext/options/options.h>

#include <config.h>
#include <lib/zone.h>
#include <lib/lv2_uris.h>

#define BUF_SIZE 513
#define SAMPLE_RATE 44100

#define JM_SAMPLER_UI_URI JM_SAMPLER_URI "#ui"

struct jm_sampler_ui {
  LV2_URID_Map* map;
  jm::uris uris;
  LV2UI_Write_Function write;
  LV2UI_Controller controller;
  LV2_Atom_Forge forge;
  // use raw fd for messages from ui
  // because we need non-blocking io
  int fdin; 
  FILE* fout;
  pid_t pid;
  int tot_read;
  char buf[BUF_SIZE];
  const std::vector<jm::zone>* zones;
  bool spawned;
};

static std::string get_time_str() {
  char time_str[9];
  time_t cur_time = time(NULL);
  struct tm* loc_time = localtime(&cur_time);
  strftime(time_str, 9, "%H:%M:%S", loc_time);
  return time_str;
}

// like strcpy but can destructively overlap
static void strmov(char* dest, char* source) {
  size_t i;

  for (i = 0; source[i] != '\0'; ++i)
    dest[i] = source[i];

  // don't forget to update null
  dest[i] = '\0';
}

static LV2_Atom* handle_update_zone(jm_sampler_ui* ui, char* params) {
  LV2_Atom_Forge_Frame obj_frame;
  LV2_Atom* obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_updateZone);
  lv2_atom_forge_key(&ui->forge, ui->uris.jm_params);
  LV2_Atom_Forge_Frame tuple_frame;
  lv2_atom_forge_tuple(&ui->forge, &tuple_frame);
  char* p = strtok(params, ",");
  int index = atoi(p);
  lv2_atom_forge_int(&ui->forge, index);
  p = strtok(NULL, ",");
  int type = atoi(p);
  lv2_atom_forge_int(&ui->forge, type);
  p = strtok(NULL, ",");

  switch (type) {
    case jm::ZONE_NAME:
    case jm::ZONE_PATH:
      lv2_atom_forge_string(&ui->forge, p, strlen(p));
      break;
    case jm::ZONE_AMP:
    case jm::ZONE_SUSTAIN:
      lv2_atom_forge_float(&ui->forge, atof(p));
      break;
    case jm::ZONE_ORIGIN:
    case jm::ZONE_LOW_KEY:
    case jm::ZONE_HIGH_KEY:
    case jm::ZONE_LOW_VEL:
    case jm::ZONE_HIGH_VEL:
    case jm::ZONE_LOOP_MODE:
    case jm::ZONE_GROUP:
    case jm::ZONE_OFF_GROUP:
    case jm::ZONE_START:
    case jm::ZONE_LEFT:
    case jm::ZONE_RIGHT:
    case jm::ZONE_CROSSFADE:
    case jm::ZONE_ATTACK:
    case jm::ZONE_HOLD:
    case jm::ZONE_DECAY:
    case jm::ZONE_RELEASE:
      lv2_atom_forge_int(&ui->forge, atoi(p));
      break;
    case jm::ZONE_PITCH:
      lv2_atom_forge_double(&ui->forge, atof(p));
      break;
  }
  lv2_atom_forge_pop(&ui->forge, &tuple_frame);
  lv2_atom_forge_pop(&ui->forge, &obj_frame);

  return obj;
}

static void add_ui_zone(jm_sampler_ui* ui, int i) {
  char outstr[256];
  memset(outstr, 0, 256);
  char* p = outstr;
  sprintf(p, "add_zone:");
  // index
  p += strlen(p);
  sprintf(p, "%i,", i);
  // wave length
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].wave_length);
  // name
  p += strlen(p);
  sprintf(p, "%s,", (*ui->zones)[i].name);
  std::cerr << "UI: adding ui zone!! " << (*ui->zones)[i].name << std::endl;
  // amp
  p += strlen(p);
  sprintf(p, "%f,", (*ui->zones)[i].amp);
  // origin
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].origin);
  // low key
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].low_key);
  // high key
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].high_key);
  // low vel
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].low_vel);
  // high vel
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].high_vel);
  // pitch
  p += strlen(p);
  sprintf(p, "%f,", (*ui->zones)[i].pitch_corr);
  // start
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].start);
  // left
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].left);
  // right
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].right);
  // loop mode
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].loop_mode);
  // crossfade
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].crossfade);
  // group
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].group);
  // off group
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].off_group);
  // attack
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].attack);
  // hold
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].hold);
  // decay
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].decay);
  // sustain
  p += strlen(p);
  sprintf(p, "%f,", (*ui->zones)[i].sustain);
  // release
  p += strlen(p);
  sprintf(p, "%i,", (*ui->zones)[i].release);
  // path
  p += strlen(p);
  sprintf(p, "%s\n", (*ui->zones)[i].path);
  fprintf(ui->fout, outstr);
  fflush(ui->fout);
}

static LV2UI_Handle instantiate(const LV2UI_Descriptor*,
    const char*, const char*,
    LV2UI_Write_Function write_function, LV2UI_Controller controller,
    LV2UI_Widget* widget, const LV2_Feature* const* features) {
  jm_sampler_ui* ui = new jm_sampler_ui;

  // Scan host features for URID map
  LV2_URID_Map* map = NULL;
  //LV2_URID_Unmap* unmap = NULL;
  //LV2_Options_Option* opt = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map))
      map = (LV2_URID_Map*)features[i]->data;
    /*else if (!strcmp(features[i]->URI, LV2_URID__unmap))
      unmap = (LV2_URID_Unmap*)features[i]->data;
    else if (!strcmp(features[i]->URI, LV2_OPTIONS__options))
      opt = (LV2_Options_Option*)features[i]->data;
    */
  }
  if (!map) {
    std::cerr << "Host does not support urid:map." << std::endl;
    delete ui;
    return NULL;
  }
  /*if (!unmap) {
    fprintf(stderr, "Host does not support urid:unmap.\n");
    free(ui);
    return NULL;
  }
  if (!opt) {
    fprintf(stderr, "Host does not support options:options\n");
    free(ui);
    return NULL;
  }

  int index = 0;
  while (1) {
    if (opt[index].key == 0)
      break;

    fprintf(stderr, "UI host option: %s\n", unmap->unmap(unmap->handle, opt[index].key));
    ++index;    
  }
  */

  // Map URIS
  ui->map = map;
  jm::map_uris(ui->map, &ui->uris);

  ui->write = write_function;
  ui->controller = controller;

  lv2_atom_forge_init(&ui->forge, ui->map);

  ui->spawned = false;

  std::cerr << get_time_str() << " UI: ui instantiated" << std::endl;

  return ui;
}

static void cleanup(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  std::cerr << "UI: " << get_time_str() <<  " cleanup called" << std::endl;

  delete ui;
}

static void port_event(LV2UI_Handle handle, uint32_t port_index,
    uint32_t, uint32_t format, const void* buffer) {

  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);
  const LV2_Atom* atom = (const LV2_Atom*) buffer;

  if (format == 0) {
    /*if (port_index == 1)
      fprintf(ui->fout, "update_vol:%f\n", *(float*) buffer);
    else if (port_index == 2)
      fprintf(ui->fout, "update_chan:%f\n", *(float*) buffer);
    fflush(ui->fout);
    */
  }
  else if (format == ui->uris.atom_eventTransfer && (atom->type == ui->uris.atom_Blank || atom->type == ui->uris.atom_Object)) {
    const LV2_Atom_Object* obj = (const LV2_Atom_Object*) atom;
    if (obj->body.otype == ui->uris.jm_getZoneVect) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      ui->zones = reinterpret_cast<const std::vector<jm::zone>*>(((LV2_Atom_Long*) params)->body);
      std::cerr << "UI: received zone pointer!! " << ui->zones << std::endl;
      for (int i = 0; i < ui->zones->size(); ++i) {
        add_ui_zone(ui, i);
      }
    }
    else if (obj->body.otype == ui->uris.jm_getSampleRate) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      int sample_rate = ((LV2_Atom_Int*) params)->body;
      std::cerr << "UI: received sample rate!! " << sample_rate << std::endl;
      fprintf(ui->fout, "set_sample_rate:%i\n", sample_rate);
      fflush(ui->fout);
    }
    else if (obj->body.otype == ui->uris.jm_addZone) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      // index
      int i = ((LV2_Atom_Int*) params)->body;

      add_ui_zone(ui, i);
    }
    else if (obj->body.otype == ui->uris.jm_removeZone) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      int index = ((LV2_Atom_Int*) params)->body;
      std::cerr << "UI: received remove zone!! " << index << std::endl;
      fprintf(ui->fout, "remove_zone:%i\n", index);
      fflush(ui->fout);
    }
    else if (obj->body.otype == ui->uris.jm_updateVol) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      float val = ((LV2_Atom_Float*) params)->body;
      ui->write(ui->controller, 1, sizeof(float), 0, &val);
    }
    else if (obj->body.otype == ui->uris.jm_updateChan) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      float val = ((LV2_Atom_Float*) params)->body;
      ui->write(ui->controller, 2, sizeof(float), 0, &val);
    }
  }
}

static int ui_show(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);
  std::cerr << "UI: " << get_time_str() <<  " show called" << std::endl;
  if (ui->spawned)
    return 0;


  int from_child_pipe[2];
  int to_child_pipe[2];

  pipe(from_child_pipe);
  pipe(to_child_pipe);

  pid_t pid = fork();

  // i'm the child
  if (pid == 0) {
    dup2(from_child_pipe[1], 1);
    close(from_child_pipe[0]);

    dup2(to_child_pipe[0], 0);
    close(to_child_pipe[1]);

    execl(CONFIG_INSTALL_PREFIX "/libexec/jm-sampler-ui", "jm-sampler-ui", NULL);
  }
  // i'm the parent
  close(from_child_pipe[1]);
  close(to_child_pipe[0]);

  fcntl(from_child_pipe[0], F_SETFL, O_NONBLOCK); 

  // close fd's subsequent children don't need on exec
  fcntl(from_child_pipe[0], F_SETFD, FD_CLOEXEC); 
  // this fd is important to close on exec so child properly gets EOF when parent closes fout
  fcntl(to_child_pipe[1], F_SETFD, FD_CLOEXEC); 

  ui->fdin = from_child_pipe[0];
  ui->fout = fdopen(to_child_pipe[1], "w");
  ui->pid = pid;
  ui->tot_read = 0;

  uint8_t buf[128];

  lv2_atom_forge_set_buffer(&ui->forge, buf, 128);
  LV2_Atom_Forge_Frame obj_frame;
  LV2_Atom* obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_getSampleRate);
  lv2_atom_forge_pop(&ui->forge, &obj_frame);

  ui->write(ui->controller, 0, lv2_atom_total_size(obj), ui->uris.atom_eventTransfer, obj);

  ui->zones = NULL;

  obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_getZoneVect);
  lv2_atom_forge_pop(&ui->forge, &obj_frame);

  ui->write(ui->controller, 0, lv2_atom_total_size(obj), ui->uris.atom_eventTransfer, obj);

  ui->spawned = true;

  std::cerr << get_time_str() << " UI: show completed" << std::endl;
  return 0;
}

static int ui_hide(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  std::cerr << "UI: " << get_time_str() <<  " hide called" << std::endl;

  fclose(ui->fout);
  waitpid(ui->pid, NULL, 0);

  std::cerr << "UI: " << get_time_str() <<  " ui closed" << std::endl;

  return 0;
}

static int ui_idle(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  //std::cerr << "UI: " << get_time_str() <<  " idle called" << std::endl;
  int num_read;
  while ((num_read = read(ui->fdin, ui->buf + ui->tot_read, BUF_SIZE - 1 - ui->tot_read)) > 0) {
    // add null char
    ui->buf[ui->tot_read + num_read] = '\0';
    char* new_line;
    while ((new_line = strchr(ui->buf, '\n')) != NULL) {
      *new_line = '\0';

      if (!strncmp(ui->buf, "update_vol:", 11)) {
        float val = atof(ui->buf + 11);
        ui->write(ui->controller, 1, sizeof(float), 0, &val);
      }
      else if (!strncmp(ui->buf, "update_chan:", 12)) {
        float val = atof(ui->buf + 12);
        ui->write(ui->controller, 2, sizeof(float), 0, &val);
      }
      else {
        uint8_t buf[128];
        lv2_atom_forge_set_buffer(&ui->forge, buf, 128);
        //fprintf(stderr, "UI: ui stdout message: %s\n", ui->buf);
        LV2_Atom* obj;
        if (!strncmp(ui->buf, "update_zone:", 12)) {
          obj = handle_update_zone(ui, ui->buf + 12);
        }
        else if (!strncmp(ui->buf, "remove_zone:", 12)) {
          //fprintf(stderr, "UI: ui remove zone: %s\n", ui->buf + 12);
          LV2_Atom_Forge_Frame obj_frame;
          obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_removeZone);
          lv2_atom_forge_key(&ui->forge, ui->uris.jm_params);
          int index = atoi(ui->buf + 12);
          lv2_atom_forge_int(&ui->forge, index);
          lv2_atom_forge_pop(&ui->forge, &obj_frame);
        }
        else if (!strncmp(ui->buf, "add_zone:", 9)) {
          //fprintf(stderr, "UI: ui add zone: %s; len: %i\n", ui->buf + 9, strlen(ui->buf + 9));
          LV2_Atom_Forge_Frame obj_frame;
          obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_addZone);
          lv2_atom_forge_key(&ui->forge, ui->uris.jm_params);
          LV2_Atom_Forge_Frame tuple_frame;
          lv2_atom_forge_tuple(&ui->forge, &tuple_frame);
          char* p = strtok(ui->buf + 9, ",");
          int index = atoi(p);
          lv2_atom_forge_int(&ui->forge, index);
          p = strtok(NULL, ",");
          lv2_atom_forge_string(&ui->forge, p, strlen(p));
          lv2_atom_forge_pop(&ui->forge, &tuple_frame);
          lv2_atom_forge_pop(&ui->forge, &obj_frame);
        }
        else if (!strncmp(ui->buf, "load_patch:", 11)) {
          //fprintf(stderr, "UI: ui add zone: %s; len: %i\n", ui->buf + 9, strlen(ui->buf + 9));
          LV2_Atom_Forge_Frame obj_frame;
          obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_loadPatch);
          lv2_atom_forge_key(&ui->forge, ui->uris.jm_params);
          lv2_atom_forge_string(&ui->forge, ui->buf + 11, strlen(ui->buf + 11));
          lv2_atom_forge_pop(&ui->forge, &obj_frame);
        }
        else if (!strncmp(ui->buf, "save_patch:", 11)) {
          LV2_Atom_Forge_Frame obj_frame;
          obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_savePatch);
          lv2_atom_forge_key(&ui->forge, ui->uris.jm_params);
          lv2_atom_forge_string(&ui->forge, ui->buf + 11, strlen(ui->buf + 11));
          lv2_atom_forge_pop(&ui->forge, &obj_frame);
        }
        ui->write(ui->controller, 0, lv2_atom_total_size(obj), ui->uris.atom_eventTransfer, obj);
      }
      // shift left to prepare next value
      strmov(ui->buf, new_line + 1);
      // correct for shift
      ui->tot_read -= new_line + 1 - ui->buf;
    }
    ui->tot_read += num_read;
  }

  // if exactly 0 the child stream is closed due to exiting
  if (num_read == 0)
    return 1;

  return 0;
}

static const void* extension_data(const char* uri) {
  static const LV2UI_Show_Interface show = { ui_show, ui_hide };
  static const LV2UI_Idle_Interface idle = { ui_idle };
  if (!strcmp(uri, LV2_UI__showInterface)) {
    return &show;
  } else if (!strcmp(uri, LV2_UI__idleInterface)) {
    return &idle;
  }
  return NULL;
}

static const LV2UI_Descriptor descriptor = {
  JM_SAMPLER_UI_URI,
  instantiate,
  cleanup,
  port_event,
  extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
