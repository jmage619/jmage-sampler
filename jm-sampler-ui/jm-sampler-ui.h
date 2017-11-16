// jm-sampler-ui.h
//
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

#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>

#include <lib/zone.h>
#include "zonegrid.h"

class HVolumeSlider;
class QComboBox;

Q_DECLARE_METATYPE(jm::zone)

class InputThread: public QThread {
  Q_OBJECT

  private:
    int sample_rate;
  public:
    void run();
  signals:
    //void receivedValue(int val);
    void receivedSampleRate(int sample_rate);
    void receivedAddZone(int i, const jm::zone& z);
    void receivedUpdateWave(int i, const QString& path, int wave_length, int start, int left, int right);
    void receivedRemoveZone(int i);
    void receivedClearZones();
    void receivedUpdateVol(double val);
    void receivedUpdateChan(int index);
};

class SamplerUI: public QWidget {
  Q_OBJECT

  private:
    HVolumeSlider* vol_slider;
    QComboBox* chan_combo;
    ZoneTableModel zone_model;

  public:
    SamplerUI();

  public slots:
    void handleUserUpdate();
    void checkAndUpdateVol(double val);
    void checkAndUpdateChan(int index);
    void sendAddZone();
    void sendLoadPatch();
    void sendSavePatch();
    void sendRefresh();
    void sendUpdateVol(double val);
    void sendUpdateChan(int index);
};

#endif
