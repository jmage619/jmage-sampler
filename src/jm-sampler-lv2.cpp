#include <cstdlib>
#include <cstdio>

#include <lv2/lv2plug.in/ns/lv2core/lv2.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>

#define JM_SAMPLER_URI "http://lv2plug.in/plugins/jm-sampler"

enum {
  SAMPLER_CONTROL = 0,
  SAMPLER_NOTIFY  = 1,
  SAMPLER_OUT_L = 2,
  SAMPLER_OUT_R = 3
};

struct jm_sampler {
  const LV2_Atom_Sequence* control_port;
  LV2_Atom_Sequence* notify_port;
  float* out_port_l;
  float* out_port_r;
};

static LV2_Handle instantiate(const LV2_Descriptor* descriptor,
    double rate, const char* path, const LV2_Feature* const* features) {
  jm_sampler* sampler = new jm_sampler;

  printf("sampler instantiated.\n");
  return sampler;
}

static void cleanup(LV2_Handle instance)
{
  delete static_cast<jm_sampler*>(instance);
}

static void connect_port(LV2_Handle instance, uint32_t port, void* data) {
  jm_sampler* sampler = static_cast<jm_sampler*>(instance);
  switch (port) {
    case SAMPLER_CONTROL:
      sampler->control_port = static_cast<const LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_NOTIFY:
      sampler->notify_port = static_cast<LV2_Atom_Sequence*>(data);
      break;
    case SAMPLER_OUT_L:
      sampler->out_port_l = (float*) data;
      break;
    case SAMPLER_OUT_R:
      sampler->out_port_r = (float*) data;
      break;
    default:
      break;
  }
}

static void run(LV2_Handle instance, uint32_t n_samples) {
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
