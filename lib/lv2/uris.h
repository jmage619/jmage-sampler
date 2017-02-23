#ifndef LV2_URIS_H
#define LV2_URIS_H

#include <stdio.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>

#define JM_SAMPLER_URI "http://lv2plug.in/plugins/jm-sampler"
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

enum zone_params {
  JM_ZONE_NAME,
  JM_ZONE_AMP,
  JM_ZONE_ORIGIN,
  JM_ZONE_LOW_KEY,
  JM_ZONE_HIGH_KEY,
  JM_ZONE_LOW_VEL,
  JM_ZONE_HIGH_VEL,
  JM_ZONE_PITCH,
  JM_ZONE_START,
  JM_ZONE_LEFT,
  JM_ZONE_RIGHT,
  JM_ZONE_LOOP_MODE,
  JM_ZONE_CROSSFADE,
  JM_ZONE_GROUP,
  JM_ZONE_OFF_GROUP,
  JM_ZONE_ATTACK,
  JM_ZONE_HOLD,
  JM_ZONE_DECAY,
  JM_ZONE_SUSTAIN,
  JM_ZONE_RELEASE,
  JM_ZONE_LONG_TAIL, // not used but needed as placeholder to not screw up PATH index
  JM_ZONE_PATH
};

typedef struct {
  LV2_URID atom_eventTransfer;
  LV2_URID atom_Blank;
  LV2_URID atom_Object;
  LV2_URID atom_String;
  LV2_URID midi_Event;
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
} jm_uris;

static inline void jm_map_uris(LV2_URID_Map* map, jm_uris* uris) {
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
	uris->atom_String = map->map(map->handle, LV2_ATOM__String);
  uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
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

#endif
