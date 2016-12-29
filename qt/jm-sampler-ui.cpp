#include <iostream>
#include <string>
#include <sstream>
#include <QtWidgets>

#include "jm-sampler-ui.h"
#include "zonegrid.h"
#include "components.h"

void InputThread::run() {
  //char input[256];
  std::string input;

  while (true) {
    std::getline(std::cin, input);
    if (std::cin.eof())
      break;

    if (input == "show:1")
      emit receivedShow();
    else if (input == "show:0")
      emit receivedHide();
    else if (!input.compare(0, 9, "add_zone:")) {
      zone z;
      std::istringstream sin(input.substr(9));
      std::string field;
      std::getline(sin, field, ',');
      z.wave_length = atof(field.c_str());
      std::getline(sin, field, ',');
      z.name = field.c_str();
      std::getline(sin, field, ',');
      z.amp = atof(field.c_str());
      std::getline(sin, field, ',');
      z.origin = note_to_string(atoi(field.c_str()));
      std::getline(sin, field, ',');
      z.low_key = note_to_string(atoi(field.c_str()));
      std::getline(sin, field, ',');
      z.high_key = note_to_string(atoi(field.c_str()));
      std::getline(sin, field, ',');
      z.low_vel = atof(field.c_str());
      std::getline(sin, field, ',');
      z.high_vel = atof(field.c_str());
      std::getline(sin, field, ',');
      z.pitch = atof(field.c_str());
      std::getline(sin, field, ',');
      z.start = atof(field.c_str());
      std::getline(sin, field, ',');
      z.left = atof(field.c_str());
      std::getline(sin, field, ',');
      z.right = atof(field.c_str());

      // loop mode
      std::getline(sin, field, ',');
      switch(atoi(field.c_str())) {
        case LOOP_OFF:
          z.loop_mode = "off";
          break;
        case LOOP_CONTINUOUS:
          z.loop_mode = "on";
          break;
        case LOOP_ONE_SHOT:
          z.loop_mode = "one shot";
          break;
      }

      std::getline(sin, field, ',');
      z.crossfade = atof(field.c_str());
      std::getline(sin, field, ',');
      z.group = field == "0" ? "none": field.c_str();
      std::getline(sin, field, ',');
      z.off_group = field == "0" ? "none": field.c_str();
      std::getline(sin, field, ',');
      z.attack = atof(field.c_str());
      std::getline(sin, field, ',');
      z.hold = atof(field.c_str());
      std::getline(sin, field, ',');
      z.decay = atof(field.c_str());
      std::getline(sin, field, ',');
      z.sustain = atof(field.c_str());
      std::getline(sin, field, ',');
      z.release = atof(field.c_str());

      z.long_tail = z.decay > 2.0 || z.release > 2.0 ? Qt::Checked: Qt::Unchecked;

      std::getline(sin, field, ',');
      z.path = field.c_str();

      emit receivedAddZone(z);
    }
  }
}

SamplerUI::SamplerUI() {
  QVBoxLayout* v_layout = new QVBoxLayout;

  ZoneTableView* view = new ZoneTableView(&zone_model);
  view->setSelectionMode(QAbstractItemView::NoSelection);

  v_layout->addWidget(view);
  QPushButton* add_button = new QPushButton("+");
  add_button->setSizePolicy(QSizePolicy::Fixed, add_button->sizePolicy().verticalPolicy());
  v_layout->addWidget(add_button);

  setLayout(v_layout);

  /*zone_model.insertRows(0, 1);
  QModelIndex i = zone_model.index(0, 0);
  zone_model.setData(i, "Zone 1", Qt::EditRole);
  i = zone_model.index(0, 1);
  zone_model.setData(i, "-10.0", Qt::EditRole);
  */
  //addNewZone({"Zone 1", "-10.0"});

  InputThread* in_thread = new InputThread;
  qRegisterMetaType<zone>();
  //connect(in_thread, &InputThread::receivedValue, slider, &QSlider::setValue);
  connect(in_thread, &InputThread::receivedShow, this, &SamplerUI::showAndRaise);
  connect(in_thread, &InputThread::receivedHide, this, &QWidget::hide);
  connect(in_thread, &InputThread::receivedAddZone, this, &SamplerUI::addNewZone);
  connect(in_thread, &QThread::finished, in_thread, &QObject::deleteLater);
  connect(in_thread, &QThread::finished, this, &QWidget::close);
  connect(in_thread, &QThread::finished, &QApplication::quit);
  in_thread->start();
}

void SamplerUI::showAndRaise() {
  show();
  raise();
  activateWindow();
}

void SamplerUI::addNewZone(const zone& z) {
  int i = zone_model.rowCount();
  zone_model.insertRows(i, 1);
  zone_model.setZone(i, z);
}
