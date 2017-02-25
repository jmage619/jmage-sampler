#include <iostream>
#include <string>
#include <sstream>
#include <QtWidgets>
#include <cmath>
#include <cstring>

#include <lib/zone.h>
#include <lib/gui_components.h>
#include <lib/gui_zonegrid.h>

#include "jm-sampler-ui.h"

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
      z.wave_length = atoi(field.c_str());
      std::getline(sin, field, ',');
      strcpy(z.name, field.c_str());
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
    else if (!input.compare(0, 12, "remove_zone:"))
      emit receivedRemoveZone(atoi(input.substr(12).c_str()));
    else if (!input.compare(0, 11, "update_vol:"))
      emit receivedUpdateVol(atof(input.substr(11).c_str()));
    else if (!input.compare(0, 12, "update_chan:"))
      emit receivedUpdateChan(atoi(input.substr(12).c_str()));
  }
}

SamplerUI::SamplerUI() {
  QVBoxLayout* v_layout = new QVBoxLayout;

  QHBoxLayout* h_layout = new QHBoxLayout;
  QPushButton* save_button = new QPushButton("save");
  connect(save_button, &QAbstractButton::clicked, this, &SamplerUI::sendSavePatch);
  h_layout->addWidget(save_button);
  QPushButton* load_button = new QPushButton("load");
  connect(load_button, &QAbstractButton::clicked, this, &SamplerUI::sendLoadPatch);
  h_layout->addWidget(load_button);
  h_layout->addStretch();
  v_layout->addLayout(h_layout);

  h_layout = new QHBoxLayout;
  QLabel* label = new QLabel;
  label->setText(tr("Vol:"));
  h_layout->addWidget(label);

  vol_slider = new HDoubleSlider(Q_NULLPTR, 0, 16, 17);
  connect(vol_slider, &HDoubleSlider::sliderMoved, this, &SamplerUI::sendUpdateVol);
  h_layout->addWidget(vol_slider);

  label = new QLabel;
  label->setText(tr("Chan:"));
  h_layout->addWidget(label);

  chan_combo = new QComboBox;
  for (int i = 1; i <= 16; ++i)
    chan_combo->addItem(QString::number(i));
  connect(chan_combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SamplerUI::sendUpdateChan);

  h_layout->addWidget(chan_combo);

  h_layout->addStretch();

  v_layout->addLayout(h_layout);
  
  ZoneTableView* view = new ZoneTableView(&zone_model);
  view->setSelectionMode(QAbstractItemView::NoSelection);

  v_layout->addWidget(view);
  QPushButton* add_button = new QPushButton("+");
  connect(add_button, &QAbstractButton::clicked, this, &SamplerUI::sendAddZone);
  v_layout->addWidget(add_button, 0, Qt::AlignLeft);

  setLayout(v_layout);

  InputThread* in_thread = new InputThread;
  qRegisterMetaType<zone>();
  connect(in_thread, &InputThread::receivedShow, this, &SamplerUI::showAndRaise);
  connect(in_thread, &InputThread::receivedHide, this, &QWidget::hide);
  connect(in_thread, &InputThread::receivedAddZone, this, &SamplerUI::addNewZone);
  connect(in_thread, &InputThread::receivedRemoveZone, this, &SamplerUI::removeZone);
  connect(in_thread, &InputThread::receivedUpdateVol, this, &SamplerUI::checkAndUpdateVol);
  connect(in_thread, &InputThread::receivedUpdateChan, this, &SamplerUI::checkAndUpdateChan);
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

void SamplerUI::removeZone(int i) {
  zone_model.removeRows(i, 1);
}

void SamplerUI::checkAndUpdateVol(double val) {
  // only set if different, 0.05 for double comparison fuzziness
  if (fabs(vol_slider->value() - val) > 0.05)
    vol_slider->setValue(val);
}

void SamplerUI::checkAndUpdateChan(int index) {
  // only set if different
  if (chan_combo->currentIndex() != index)
    chan_combo->setCurrentIndex(index);
}

void SamplerUI::sendAddZone() {
  QString path = QFileDialog::getOpenFileName(this, tr("Open a FUCKING WAV already!!"), "", tr("WAV (*.wav)"));
  if (!path.isNull())
    std::cout << "add_zone:" << path.toStdString() << std::endl;
}

void SamplerUI::sendLoadPatch() {
  QString path = QFileDialog::getOpenFileName(this, tr("Open a FUCKING patch already!!"), "", tr("Patch Files (*.sfz *.jmz);;SFZ (*.sfz);;JMZ (*.jmz)"));
  if (!path.isNull())
    std::cout << "load_patch:" << path.toStdString() << std::endl;
}

void SamplerUI::sendSavePatch() {
  QString path = QFileDialog::getSaveFileName(this, tr("Save a FUCKING patch already!!"), "", tr("Patch Files (*.sfz *.jmz);;SFZ (*.sfz);;JMZ (*.jmz)"));
  if (!path.isNull())
    std::cout << "save_patch:" << path.toStdString() << std::endl;
}

void SamplerUI::sendUpdateVol(double val) {
  std::cout << "update_vol:" << val << std::endl;
}

void SamplerUI::sendUpdateChan(int index) {
  std::cout << "update_chan:" << index << std::endl;
}