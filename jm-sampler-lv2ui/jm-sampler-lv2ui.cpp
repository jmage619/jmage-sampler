/****************************************************************************
    Copyright (C) 2017  jmage619

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

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
using std::cerr;
using std::endl;

#include <string>
#include <vector>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/forge.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/instance-access/instance-access.h>

#include <config.h>
#include <lib/zone.h>
#include <lib/lv2_uris.h>
#include <lib/jmsampler.h>
#include <lib/lv2sampler.h>
#include "lv2_external_ui.h"

#define BUF_SIZE 513
#define SAMPLE_RATE 44100

#define JM_SAMPLER_UI_URI JM_SAMPLER_URI "#ui"

struct jm_sampler_ui {
  LV2_External_UI_Widget widget;
  LV2Sampler* sampler;
  LV2_URID_Map* map;
  jm::uris uris;
  LV2UI_Write_Function write;
  LV2UI_Controller controller;
  LV2_Atom_Forge forge;
  LV2_External_UI_Host* host;
  // use raw fd for messages from ui
  // because we need non-blocking io
  int fdin; 
  pid_t pid;
  int tot_read;
  char buf[BUF_SIZE];
  char title[256];
  bool spawned;
  float volume;
  float channel;
};

/*static std::string get_time_str() {
  char time_str[9];
  time_t cur_time = time(NULL);
  struct tm* loc_time = localtime(&cur_time);
  strftime(time_str, 9, "%H:%M:%S", loc_time);
  return time_str;
}
*/

// like strcpy but can destructively overlap
static void strmov(char* dest, char* source) {
  size_t i;

  for (i = 0; source[i] != '\0'; ++i)
    dest[i] = source[i];

  // don't forget to update null
  dest[i] = '\0';
}

static int ui_show(jm_sampler_ui* ui) {
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

    if (strlen(ui->title) == 0)
      execl(CONFIG_INSTALL_PREFIX "/libexec/jm-sampler-ui", "jm-sampler-ui", NULL);
    else
      execl(CONFIG_INSTALL_PREFIX "/libexec/jm-sampler-ui", "jm-sampler-ui", ui->title, NULL);
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
  ui->sampler->fout = fdopen(to_child_pipe[1], "w");
  ui->pid = pid;
  ui->tot_read = 0;

  fprintf(ui->sampler->fout, "set_sample_rate:%i\n", ui->sampler->sample_rate);
  fflush(ui->sampler->fout);

  fprintf(ui->sampler->fout, "update_vol:%f\n", ui->volume);
  fprintf(ui->sampler->fout, "update_chan:%f\n", ui->channel);
  fflush(ui->sampler->fout);

  for (int i = 0; i < ui->sampler->zones.size(); ++i)
    ui->sampler->send_add_zone(i);

  ui->spawned = true;

  //cerr << get_time_str() << " UI: show completed" << endl;
  return 0;
}

static int ui_run(jm_sampler_ui* ui) {
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
      else if (!strncmp(ui->buf, "update_zone:", 12)) {
          char* p = strtok(ui->buf + 12, ",");
          int index = atoi(p);
          p = strtok(NULL, ",");
          int key = atoi(p);

          p = strtok(NULL, ",");

          // special case, update wave
          if (key == jm::ZONE_PATH) {
            if (ui->sampler->waves.find(p) == ui->sampler->waves.end()) {
              ui->sampler->waves[p] = jm::parse_wave(p);
            }
          }

          ui->sampler->update_zone(index, key, p);
      }
      else if (!strncmp(ui->buf, "remove_zone:", 12)) {
        int index = atoi(ui->buf + 12);
        ui->sampler->remove_zone(index);

        fprintf(ui->sampler->fout, "remove_zone:%i\n", index);
        fflush(ui->sampler->fout);
      }
      else if (!strncmp(ui->buf, "add_zone:", 9)) {
        char* p = strtok(ui->buf + 9, ",");
        int index = atoi(p);
        p = strtok(NULL, ",");

        if (ui->sampler->waves.find(p) == ui->sampler->waves.end()) {
          ui->sampler->waves[p] = jm::parse_wave(p);
        }

        ui->sampler->add_zone_from_wave(index, p);
      }
      else if (!strncmp(ui->buf, "dup_zone:", 9)) {
        char* p = strtok(ui->buf + 9, ",");
        int index = atoi(p);
        ui->sampler->duplicate_zone(index);
      }
      else if (!strncmp(ui->buf, "save_patch:", 11)) {
        ui->sampler->save_patch(ui->buf + 11);
      }
      else if (!strncmp(ui->buf, "refresh", 7)) {
        ui->sampler->reload_waves();
      }
      else {
         uint8_t buf[128];
         lv2_atom_forge_set_buffer(&ui->forge, buf, 128);
         //cerr << "UI: ui stdout message: " << ui->buf << endl;
         LV2_Atom* obj;

        if (!strncmp(ui->buf, "load_patch:", 11)) {
          LV2_Atom_Forge_Frame obj_frame;
          obj = (LV2_Atom*) lv2_atom_forge_object(&ui->forge, &obj_frame, 0, ui->uris.jm_loadPatch);
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
  if (num_read == 0) {
    fclose(ui->sampler->fout);
    waitpid(ui->pid, NULL, 0);

    ui->spawned = false;

    return 1;
  }

  return 0;
}

static void ext_show(LV2_External_UI_Widget* widget) {
  jm_sampler_ui* ui = reinterpret_cast<jm_sampler_ui*>(widget);
  //cerr << "UI: " << get_time_str() <<  "ext show called" << endl;
  ui_show(ui);
}

static void ext_hide(LV2_External_UI_Widget* widget) {
  //cerr << "UI: " << get_time_str() <<  "ext hide called" << endl;
}

static void ext_run(LV2_External_UI_Widget* widget) {
  jm_sampler_ui* ui = reinterpret_cast<jm_sampler_ui*>(widget);
  //cerr << "UI: " << get_time_str() <<  "ext run called" << endl;
  if (ui_run(ui))
    ui->host->ui_closed(ui->controller);
}

static int show(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);
  //cerr << "UI: " << get_time_str() <<  " show called" << endl;
  return ui_show(ui);
}

static int hide(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  //cerr << "UI: " << get_time_str() <<  " hide called" << endl;

  return 0;
}

static int idle(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  //cerr << "UI: " << get_time_str() <<  " idle called" << endl;
  return ui_run(ui);
}

static LV2UI_Handle instantiate(const LV2UI_Descriptor*,
    const char*, const char*,
    LV2UI_Write_Function write_function, LV2UI_Controller controller,
    LV2UI_Widget* widget, const LV2_Feature* const* features) {
  jm_sampler_ui* ui = new jm_sampler_ui;

  // Scan host features for URID map
  LV2_URID_Map* map = NULL;
  //LV2_URID_Unmap* unmap = NULL;
  LV2_Options_Option* opt = NULL;

  for (int i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map))
      map = (LV2_URID_Map*)features[i]->data;
    //else if (!strcmp(features[i]->URI, LV2_URID__unmap))
    //  unmap = (LV2_URID_Unmap*)features[i]->data;
    else if (!strcmp(features[i]->URI, LV2_OPTIONS__options))
      opt = (LV2_Options_Option*)features[i]->data;
    else if (!strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI)) {
      ui->sampler = reinterpret_cast<LV2Sampler*>(features[i]->data);
    }
    else if (!strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host)) {
      ui->host = reinterpret_cast<LV2_External_UI_Host*>(features[i]->data);
    }
  }
  if (!map) {
    //cerr << "Host does not support urid:map." << endl;
    delete ui;
    return NULL;
  }
  if (!ui->sampler) {
    //cerr << "Host does not support urid:map." << endl;
    delete ui;
    return NULL;
  }
  /*if (!unmap) {
    cerr << "Host does not support urid:unmap." << endl;
    free(ui);
    return NULL;
  }
  */

  // Map URIS
  ui->map = map;
  jm::map_uris(ui->map, &ui->uris);

  int index = 0;

  if (opt != NULL) {
    while (1) {
      if (opt[index].key == 0)
        break;

      //cerr << "UI host option: " << unmap->unmap(unmap->handle, opt[index].key) << endl;
      if (opt[index].key == ui->uris.ui_windowTitle) {
        //cerr << "UI window title: " <<  (const char*) opt[index].value << endl;
        strcpy(ui->title, (const char*) opt[index].value);
        break;
      }
      else
        ui->title[0] = '\0';

      ++index;
    }
  }

  ui->write = write_function;
  ui->controller = controller;

  ui->widget.show = ext_show;
  ui->widget.hide = ext_hide;
  ui->widget.run = ext_run;
  *widget = ui;

  lv2_atom_forge_init(&ui->forge, ui->map);

  ui->spawned = false;
  ui->volume = 0.f;
  ui->channel = 0;

  //cerr << get_time_str() << " UI: ui instantiated" << endl;

  return ui;
}

static void cleanup(LV2UI_Handle handle) {
  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  //cerr << "UI: " << get_time_str() <<  " cleanup called" << endl;

  delete ui;
}

static void port_event(LV2UI_Handle handle, uint32_t port_index,
    uint32_t, uint32_t format, const void* buffer) {

  jm_sampler_ui* ui = static_cast<jm_sampler_ui*>(handle);

  // store vals in case vol or chan sent before show called
  if (format == 0) {
    if (port_index == 1) {
      ui->volume = *(float*) buffer;
      if (ui->spawned) {
        fprintf(ui->sampler->fout, "update_vol:%f\n", ui->volume);
        fflush(ui->sampler->fout);
      }
    }
    else if (port_index == 2) {
      ui->channel = *(float*) buffer;
      if (ui->spawned) {
        fprintf(ui->sampler->fout, "update_chan:%f\n", ui->channel);
        fflush(ui->sampler->fout);
      }
    }
  }
}

static const void* extension_data(const char* uri) {
  static const LV2UI_Show_Interface show_interface = { show, hide };
  static const LV2UI_Idle_Interface idle_interface = { idle };
  if (!strcmp(uri, LV2_UI__showInterface)) {
    return &show_interface;
  } else if (!strcmp(uri, LV2_UI__idleInterface)) {
    return &idle_interface;
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
