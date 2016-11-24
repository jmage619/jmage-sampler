#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/atom/util.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include "lv2_external_ui.h"
#include "uris.h"

#define BUF_SIZE 513

#define JM_SAMPLER_UI_URI JM_SAMPLER_URI "#ui"

typedef struct {
  LV2_External_UI_Widget widget;
  LV2_URID_Map* map;
  jm_uris uris;
  LV2UI_Write_Function write;
  LV2UI_Controller controller;
  // use raw fd for messages from ui
  // because we need non-blocking io
  int fdin; 
  FILE* fout;
  pid_t pid;
  int tot_read;
  char buf[BUF_SIZE];
} jm_sampler_ui;

static void set_time_str(char* time_str) {
  time_t cur_time = time(NULL);
  struct tm* loc_time = localtime(&cur_time);
  strftime(time_str, 9, "%H:%M:%S", loc_time);
}

// like strcpy but can destructively overlap
static void strmov(char* dest, char* source) {
  size_t i;

  for (i = 0; source[i] != '\0'; ++i)
    dest[i] = source[i];

  // don't forget to update null
  dest[i] = '\0';
}

static void run(LV2_External_UI_Widget* widget) {
  jm_sampler_ui* ui = (jm_sampler_ui*) widget;

  int num_read;
  while ((num_read = read(ui->fdin, ui->buf + ui->tot_read, BUF_SIZE - 1 - ui->tot_read)) > 0) {
    // add null char
    ui->buf[ui->tot_read + num_read] = '\0';

    char* new_line;
    while ((new_line = strchr(ui->buf, '\n')) != NULL) {
      *new_line = '\0';
      //fprintf(stderr, "here's the value: %s\n", ui->buf);
      float val = atof(ui->buf);
      ui->write(ui->controller, 0, sizeof(float), 0, &val);

      // shift left to prepare next value
      strmov(ui->buf, new_line + 1);
      // correct for shift
      ui->tot_read -= new_line + 1 - ui->buf;
    }
    ui->tot_read += num_read;
  }
}

static void show(LV2_External_UI_Widget* widget) {
  char time_str[9];
  set_time_str(time_str);
  fprintf(stderr, "%s show called\n", time_str); 
  jm_sampler_ui* ui = (jm_sampler_ui*) widget;
  fprintf(ui->fout, "show:1\n");
  fflush(ui->fout);
}

static void hide(LV2_External_UI_Widget* widget) {
  char time_str[9];
  set_time_str(time_str);
  fprintf(stderr, "%s hide called\n", time_str); 
  jm_sampler_ui* ui = (jm_sampler_ui*) widget;
  fprintf(ui->fout, "show:0\n");
  fflush(ui->fout);
}

static LV2UI_Handle instantiate(const LV2UI_Descriptor* descriptor,
    const char* plugin_uri, const char* bundle_path,
    LV2UI_Write_Function write_function, LV2UI_Controller controller,
    LV2UI_Widget* widget, const LV2_Feature* const* features) {
  jm_sampler_ui* ui = malloc(sizeof(jm_sampler_ui));

  // Scan host features for URID map
  LV2_URID_Map* map = NULL;
  int i;
  for (i = 0; features[i]; ++i) {
    if (!strcmp(features[i]->URI, LV2_URID__map)) {
      map = (LV2_URID_Map*)features[i]->data;
    }
  }
  if (!map) {
    fprintf(stderr, "Host does not support urid:map.\n");
    free(ui);
    return NULL;
  }

  // Map URIS
  ui->map = map;
  jm_map_uris(ui->map, &ui->uris);

  ui->write = write_function;
  ui->controller = controller;
  ui->widget.run = run;
  ui->widget.show = show;
  ui->widget.hide = hide;
  *widget = ui;

  char time_str[9];
  set_time_str(time_str);
  fprintf(stderr, "%s sampler instantiate called\n", time_str);

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

    execl("/home/jdost/dev/c/jmage-sampler/qt/jm-sampler-ui", "/home/jdost/dev/c/jmage-sampler/qt/jm-sampler-ui", NULL);
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

  set_time_str(time_str);
  fprintf(stderr, "%s sampler instantiated\n", time_str);

  return ui;
}

static void cleanup(LV2UI_Handle handle) {
  jm_sampler_ui* ui = (jm_sampler_ui*) handle;
  char time_str[9];
  set_time_str(time_str);
  fprintf(stderr, "%s closing sampler\n", time_str);

  fclose(ui->fout);
  waitpid(ui->pid, NULL, 0);

  set_time_str(time_str);
  fprintf(stderr, "%s sampler closed\n", time_str);
  free(handle);
}

static void port_event(LV2UI_Handle handle, uint32_t port_index,
    uint32_t buffer_size, uint32_t format, const void* buffer) {
  jm_sampler_ui* ui = (jm_sampler_ui*) handle;
  const LV2_Atom* atom = (const LV2_Atom*) buffer;

  if (format == ui->uris.atom_eventTransfer && (atom->type == ui->uris.atom_Blank || atom->type == ui->uris.atom_Object)) {
    const LV2_Atom_Object* obj = (const LV2_Atom_Object*) atom;
    if (obj->body.otype == ui->uris.jm_addZone) {
      LV2_Atom* params = NULL;

      lv2_atom_object_get(obj, ui->uris.jm_params, &params, 0);

      char outstr[256];
      memset(outstr, 0, 256);
      char* p = outstr;
      sprintf(p, "add_zone:");
      p += strlen(p);
      LV2_Atom* a = lv2_atom_tuple_begin((LV2_Atom_Tuple*) params);
      sprintf(p, "%s,", (char*)(a + 1));
      p += strlen(p);
      a = lv2_atom_tuple_next(a);
      sprintf(p, "%s\n", (char*)(a + 1));
      fprintf(ui->fout, outstr);
      fflush(ui->fout);
      //fprintf(ui->fout, "show:1\n");
    }
  }
}
static const LV2UI_Descriptor descriptor = {
  JM_SAMPLER_UI_URI,
  instantiate,
  cleanup,
  port_event,
  NULL // extension_data
};

LV2_SYMBOL_EXPORT const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index) {
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
