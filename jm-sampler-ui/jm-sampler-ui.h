#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>

#include <lib/zone.h>
#include <lib/gui_zonegrid.h>

class HDoubleSlider;
class QComboBox;

Q_DECLARE_METATYPE(jm_zone)

class InputThread: public QThread {
  Q_OBJECT

  private:
    int sample_rate;
  public:
    InputThread(int sample_rate, QObject* parent = Q_NULLPTR): QThread(parent), sample_rate(sample_rate) {}
    void run();
  signals:
    //void receivedValue(int val);
    void receivedShow();
    void receivedHide();
    void receivedAddZone(const jm_zone& z);
    void receivedRemoveZone(int i);
    void receivedUpdateVol(double val);
    void receivedUpdateChan(int index);
};

class SamplerUI: public QWidget {
  Q_OBJECT

  private:
    HDoubleSlider* vol_slider;
    QComboBox* chan_combo;
    ZoneTableModel zone_model;

  public:
    SamplerUI(int sample_rate, QWidget* parent = Q_NULLPTR);

  public slots:
    void showAndRaise();
    void addNewZone(const jm_zone& z);
    void removeZone(int i);
    void checkAndUpdateVol(double val);
    void checkAndUpdateChan(int index);
    void sendAddZone();
    void sendLoadPatch();
    void sendSavePatch();
    void sendUpdateVol(double val);
    void sendUpdateChan(int index);
};

#endif
