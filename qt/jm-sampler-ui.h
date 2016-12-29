#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>

#include "zonegrid.h"

class InputThread: public QThread {
  Q_OBJECT

  public:
    void run();
  signals:
    //void receivedValue(int val);
    void receivedShow();
    void receivedHide();
    void receivedAddZone(const zone& z);
};

class SamplerUI: public QWidget {
  Q_OBJECT

  private:
    ZoneTableModel zone_model;

  public:
    SamplerUI();

  public slots:
    void showAndRaise();
    void addNewZone(const zone& z);
};

#endif
