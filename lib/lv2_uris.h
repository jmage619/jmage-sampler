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

#ifndef LV2_URIS_H
#define LV2_URIS_H

#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/midi/midi.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>

// nominal not defined until 1.14
#ifndef LV2_BUF_SIZE__nominalBlockLength
#define LV2_BUF_SIZE__nominalBlockLength LV2_BUF_SIZE_PREFIX "nominalBlockLength"
#endif

#define JM_SAMPLER_URI "https://github.com/jmage619/jmage-sampler"
#define JM_SAMPLER__params JM_SAMPLER_URI "#params"
#define JM_SAMPLER__loadPatch JM_SAMPLER_URI "#loadPatch"
#define JM_SAMPLER__patchFile JM_SAMPLER_URI "#patchFile"

namespace jm {
  struct uris {
    LV2_URID atom_eventTransfer;
    LV2_URID atom_Object;
    LV2_URID atom_String;
    LV2_URID midi_Event;
    LV2_URID bufsize_maxBlockLength;
    LV2_URID bufsize_nominalBlockLength;
    LV2_URID ui_windowTitle;
    LV2_URID jm_params;
    LV2_URID jm_loadPatch;
    LV2_URID jm_patchFile;
  };

  static inline void map_uris(LV2_URID_Map* map, jm::uris* uris) {
    uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
    uris->atom_Object = map->map(map->handle, LV2_ATOM__Object);
    uris->atom_String = map->map(map->handle, LV2_ATOM__String);
    uris->midi_Event = map->map(map->handle, LV2_MIDI__MidiEvent);
    uris->bufsize_maxBlockLength = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
    uris->bufsize_nominalBlockLength = map->map(map->handle, LV2_BUF_SIZE__nominalBlockLength);
    uris->ui_windowTitle = map->map(map->handle, LV2_UI__windowTitle);
    uris->jm_params = map->map(map->handle, JM_SAMPLER__params);
    uris->jm_loadPatch = map->map(map->handle, JM_SAMPLER__loadPatch);
    uris->jm_patchFile = map->map(map->handle, JM_SAMPLER__patchFile);
  }
};

#endif
