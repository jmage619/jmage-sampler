// jm-sampler-ui.cpp
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

#include <iostream>
#include <string>
#include <sstream>
#include <QtWidgets>
#include <cmath>
#include <cstring>

#include <lib/zone.h>
#include "components.h"
#include "zonegrid.h"

#include "jm-sampler-ui.h"

void InputThread::run() {
  //char input[256];
  std::string input;

  while (true) {
    std::getline(std::cin, input);
    if (std::cin.eof())
      break;

    if (!input.compare(0, 16, "set_sample_rate:")) {
      sample_rate = atoi(input.substr(16).c_str());
      emit receivedSampleRate(sample_rate);
    }
    else if (!input.compare(0, 9, "add_zone:")) {
      jm::zone z;
      std::istringstream sin(input.substr(9));
      std::string field;
      std::getline(sin, field, ',');
      int index = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.wave_length = atoi(field.c_str());
      std::getline(sin, field, ',');
      strcpy(z.name, field.c_str());
      std::getline(sin, field, ',');
      z.amp = atof(field.c_str());
      std::getline(sin, field, ',');
      z.mute = atoi(field.c_str()) ? Qt::Checked: Qt::Unchecked;
      std::getline(sin, field, ',');
      z.solo = atoi(field.c_str()) ? Qt::Checked: Qt::Unchecked;
      std::getline(sin, field, ',');
      z.origin = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.low_key = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.high_key = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.low_vel = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.high_vel = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.pitch_corr = atof(field.c_str());
      std::getline(sin, field, ',');
      z.start = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.left = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.right = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.loop_mode = (jm::loop_mode) atoi(field.c_str());
      std::getline(sin, field, ',');
      z.crossfade = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.group = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.off_group = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.attack = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.hold = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.decay = atoi(field.c_str());
      std::getline(sin, field, ',');
      z.sustain = atof(field.c_str());
      std::getline(sin, field, ',');
      z.release = atoi(field.c_str());

      z.long_tail = z.decay > 2 * sample_rate || z.release > 2 * sample_rate ? Qt::Checked: Qt::Unchecked;

      std::getline(sin, field, ',');
      strcpy(z.path, field.c_str());

      emit receivedAddZone(index, z);
    }
    else if (!input.compare(0, 12, "update_wave:")) {
      std::istringstream sin(input.substr(12));
      std::string field;
      std::getline(sin, field, ',');
      int index = atoi(field.c_str());
      std::getline(sin, field, ',');
      QString path = QString::fromStdString(field);
      std::getline(sin, field, ',');
      int wave_length = atoi(field.c_str());
      std::getline(sin, field, ',');
      int start = atoi(field.c_str());
      std::getline(sin, field, ',');
      int left = atoi(field.c_str());
      std::getline(sin, field, ',');
      int right = atoi(field.c_str());

      emit receivedUpdateWave(index, path, wave_length, start, left, right);
    }
    else if (!input.compare(0, 12, "remove_zone:"))
      emit receivedRemoveZone(atoi(input.substr(12).c_str()));
    else if (!input.compare(0, 11, "clear_zones"))
      emit receivedClearZones();
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
  save_button->setFocusPolicy(Qt::NoFocus);
  connect(save_button, &QAbstractButton::clicked, this, &SamplerUI::sendSavePatch);
  h_layout->addWidget(save_button);
  QPushButton* load_button = new QPushButton("load");
  load_button->setFocusPolicy(Qt::NoFocus);
  connect(load_button, &QAbstractButton::clicked, this, &SamplerUI::sendLoadPatch);
  h_layout->addWidget(load_button);
  QPushButton* refresh_button = new QPushButton("refresh");
  refresh_button->setFocusPolicy(Qt::NoFocus);
  connect(refresh_button, &QAbstractButton::clicked, this, &SamplerUI::sendRefresh);
  h_layout->addWidget(refresh_button);
  h_layout->addStretch();
  v_layout->addLayout(h_layout);

  h_layout = new QHBoxLayout;
  QLabel* label = new QLabel;
  label->setText(tr("Vol:"));
  h_layout->addWidget(label);

  vol_slider = new HVolumeSlider(Q_NULLPTR);
  connect(vol_slider, &HVolumeSlider::sliderMoved, this, &SamplerUI::sendUpdateVol);
  connect(vol_slider, &HVolumeSlider::sliderMoved, this, &SamplerUI::handleUserUpdate);
  h_layout->addWidget(vol_slider);

  label = new QLabel;
  label->setText(tr("Chan:"));
  h_layout->addWidget(label);

  chan_combo = new QComboBox;
  chan_combo->setFocusPolicy(Qt::NoFocus);
  for (int i = 1; i <= 16; ++i)
    chan_combo->addItem(QString::number(i));
  connect(chan_combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SamplerUI::sendUpdateChan);
  connect(chan_combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SamplerUI::handleUserUpdate);

  h_layout->addWidget(chan_combo);

  h_layout->addStretch();

  v_layout->addLayout(h_layout);
  
  ZoneTableView* view = new ZoneTableView(&zone_model);
  view->setFocusPolicy(Qt::NoFocus);
  view->setSelectionMode(QAbstractItemView::NoSelection);

  v_layout->addWidget(view);

  connect(view, &MouseHandleView::userUpdate, this, &SamplerUI::handleUserUpdate);
  QPushButton* add_button = new QPushButton("+");
  add_button->setFocusPolicy(Qt::NoFocus);
  connect(add_button, &QAbstractButton::clicked, this, &SamplerUI::sendAddZone);
  v_layout->addWidget(add_button, 0, Qt::AlignLeft);

  setLayout(v_layout);

  InputThread* in_thread = new InputThread;
  qRegisterMetaType<jm::zone>();
  connect(in_thread, &InputThread::receivedSampleRate, &zone_model, &ZoneTableModel::setSampleRate);
  connect(in_thread, &InputThread::receivedAddZone, &zone_model, &ZoneTableModel::addNewZone);
  connect(in_thread, &InputThread::receivedUpdateWave, &zone_model, &ZoneTableModel::updateWave);
  connect(in_thread, &InputThread::receivedRemoveZone, &zone_model, &ZoneTableModel::removeZone);
  connect(in_thread, &InputThread::receivedClearZones, &zone_model, &ZoneTableModel::clearZones);
  connect(in_thread, &InputThread::receivedUpdateVol, this, &SamplerUI::checkAndUpdateVol);
  connect(in_thread, &InputThread::receivedUpdateChan, this, &SamplerUI::checkAndUpdateChan);
  connect(in_thread, &QThread::finished, in_thread, &QObject::deleteLater);
  connect(in_thread, &QThread::finished, this, &QWidget::close);
  connect(in_thread, &QThread::finished, &QApplication::quit);
  in_thread->start();
}

void SamplerUI::handleUserUpdate() {
  setWindowModified(true);
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
  QString path = QFileDialog::getOpenFileName(this, tr("Open a fucking WAV already!!"), "", tr("Sound Files (*.wav *.aiff *.flac)"));
  if (!path.isNull()) {
    std::cout << "add_zone:-1," << path.toStdString() << std::endl;
    setWindowModified(true);
  }
}

void SamplerUI::sendLoadPatch() {
  QString path = QFileDialog::getOpenFileName(this, tr("Open a fucking PATCH already!!"), "", tr("Patch Files (*.sfz *.jmz);;SFZ (*.sfz);;JMZ (*.jmz)"));
  if (!path.isNull()) {
    std::cout << "load_patch:" << path.toStdString() << std::endl;
    setWindowModified(false);
  }
}

void SamplerUI::sendSavePatch() {
  QString path = QFileDialog::getSaveFileName(this, tr("Save a fucking PATCH already!!"), "", tr("Patch Files (*.sfz *.jmz);;SFZ (*.sfz);;JMZ (*.jmz)"));
  if (!path.isNull()) {
    std::cout << "save_patch:" << path.toStdString() << std::endl;
    setWindowModified(false);
  }
}

void SamplerUI::sendRefresh() {
  std::cout << "refresh" << std::endl;
}

void SamplerUI::sendUpdateVol(double val) {
  std::cout << "update_vol:" << val << std::endl;
}

void SamplerUI::sendUpdateChan(int index) {
  std::cout << "update_chan:" << index << std::endl;
}
