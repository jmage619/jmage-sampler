#ifndef JM_SAMPLER_URIS_H
#define JM_SAMPLER_URIS_H

#include <stdio.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>

#define JM_SAMPLER_URI "http://lv2plug.in/plugins/jm-sampler"
#define JM_SAMPLER__addZone JM_SAMPLER_URI "#addZone"
#define JM_SAMPLER__params JM_SAMPLER_URI "#params"
#define JM_SAMPLER__getZones JM_SAMPLER_URI "#getZones"
#define JM_SAMPLER__updateZone JM_SAMPLER_URI "#updateZone"

enum zone_params {
  JM_ZONE_NAME,
  JM_ZONE_AMP,
  JM_ZONE_ORIGIN,
  JM_ZONE_LOW_KEY,
  JM_ZONE_HIGH_KEY
};

typedef struct {
  LV2_URID atom_eventTransfer;
  LV2_URID atom_Blank;
  LV2_URID atom_Object;
  LV2_URID midi_Event;
  LV2_URID jm_addZone;
  LV2_URID jm_params;
  LV2_URID jm_getZones;
  LV2_URID jm_updateZone;
} jm_uris;

static inline void jm_map_uris(LV2_URID_Map* map, jm_uris* uris) {
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->atom_Blank = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
  uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
  uris->jm_addZone = map->map(map->handle, JM_SAMPLER__addZone);
  uris->jm_params = map->map(map->handle, JM_SAMPLER__params);
  uris->jm_getZones = map->map(map->handle, JM_SAMPLER__getZones);
  uris->jm_updateZone = map->map(map->handle, JM_SAMPLER__updateZone);
}

#endif
