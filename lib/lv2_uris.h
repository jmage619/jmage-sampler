#ifndef LV2_URIS_H
#define LV2_URIS_H

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>

// nominal not defined until 1.14
#ifndef LV2_BUF_SIZE__nominalBlockLength
#define LV2_BUF_SIZE__nominalBlockLength LV2_BUF_SIZE_PREFIX "nominalBlockLength"
#endif

#define JM_SAMPLER_URI "https://github.com/jmage619/jmage-sampler"
#define JM_SAMPLER__getSampleRate JM_SAMPLER_URI "#setSampleRate"
#define JM_SAMPLER__addZone JM_SAMPLER_URI "#addZone"
#define JM_SAMPLER__removeZone JM_SAMPLER_URI "#removeZone"
#define JM_SAMPLER__params JM_SAMPLER_URI "#params"
#define JM_SAMPLER__getZones JM_SAMPLER_URI "#getZones"
#define JM_SAMPLER__updateZone JM_SAMPLER_URI "#updateZone"
#define JM_SAMPLER__loadPatch JM_SAMPLER_URI "#loadPatch"
#define JM_SAMPLER__savePatch JM_SAMPLER_URI "#savePatch"
#define JM_SAMPLER__patchFile JM_SAMPLER_URI "#patchFile"
#define JM_SAMPLER__updateVol JM_SAMPLER_URI "#updateVol"
#define JM_SAMPLER__updateChan JM_SAMPLER_URI "#updateChan"

namespace jm {
  struct uris {
    LV2_URID atom_eventTransfer;
    LV2_URID atom_Blank;
    LV2_URID atom_Object;
    LV2_URID atom_String;
    LV2_URID midi_Event;
    LV2_URID bufsize_maxBlockLength;
    LV2_URID bufsize_nominalBlockLength;
    LV2_URID jm_getSampleRate;
    LV2_URID jm_addZone;
    LV2_URID jm_removeZone;
    LV2_URID jm_params;
    LV2_URID jm_getZones;
    LV2_URID jm_updateZone;
    LV2_URID jm_loadPatch;
    LV2_URID jm_savePatch;
    LV2_URID jm_patchFile;
    LV2_URID jm_updateVol;
    LV2_URID jm_updateChan;
  };

  static inline void map_uris(LV2_URID_Map* map, jm::uris* uris) {
    uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris->atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
    uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_String = map->map(map->handle, LV2_ATOM__String);
    uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris->bufsize_maxBlockLength = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
    uris->bufsize_nominalBlockLength = map->map(map->handle, LV2_BUF_SIZE__nominalBlockLength);
    uris->jm_getSampleRate = map->map(map->handle, JM_SAMPLER__getSampleRate);
    uris->jm_addZone = map->map(map->handle, JM_SAMPLER__addZone);
    uris->jm_removeZone = map->map(map->handle, JM_SAMPLER__removeZone);
    uris->jm_params = map->map(map->handle, JM_SAMPLER__params);
    uris->jm_getZones = map->map(map->handle, JM_SAMPLER__getZones);
    uris->jm_updateZone = map->map(map->handle, JM_SAMPLER__updateZone);
    uris->jm_loadPatch = map->map(map->handle, JM_SAMPLER__loadPatch);
    uris->jm_savePatch = map->map(map->handle, JM_SAMPLER__savePatch);
    uris->jm_patchFile = map->map(map->handle, JM_SAMPLER__patchFile);
    uris->jm_updateVol = map->map(map->handle, JM_SAMPLER__updateVol);
    uris->jm_updateChan = map->map(map->handle, JM_SAMPLER__updateChan);
  }
};

#endif
