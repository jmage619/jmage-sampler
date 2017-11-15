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
