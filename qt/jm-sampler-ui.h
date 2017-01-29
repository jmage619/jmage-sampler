#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>

#include "zonegrid.h"

class HDoubleSlider;

class InputThread: public QThread {
  Q_OBJECT

  public:
    void run();
  signals:
    //void receivedValue(int val);
    void receivedShow();
    void receivedHide();
    void receivedAddZone(const zone& z);
    void receivedRemoveZone(int i);
    void receivedUpdateVol(double val);
};

class SamplerUI: public QWidget {
  Q_OBJECT

  private:
    HDoubleSlider* slider;
    ZoneTableModel zone_model;

  public:
    SamplerUI();

  public slots:
    void showAndRaise();
    void addNewZone(const zone& z);
    void removeZone(int i);
    void checkAndUpdateVol(double val);
    void sendAddZone();
    void sendLoadPatch();
    void sendUpdateVol(double val);
};

#endif
