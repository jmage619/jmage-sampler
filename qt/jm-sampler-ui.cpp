#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <QtWidgets>
#include <unistd.h>

#include "jm-sampler-ui.h"

int ZoneTableModel::rowCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return zones.size();
}

int ZoneTableModel::columnCount(const QModelIndex& parent) const {
  if (parent.isValid())
    return 0;

  return NUM_ZONE_ATTRS;
}

// just set same for all indexes for now
Qt::ItemFlags ZoneTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool ZoneTableModel::insertRows(int row, int count, const QModelIndex&) {
  beginInsertRows(QModelIndex(), row, row + count - 1);

  std::vector<zone>::iterator first = zones.begin() + row;
  zones.insert(first, count, zone());

  endInsertRows();
  return true;
}

bool ZoneTableModel::removeRows(int row, int count, const QModelIndex&) {
  beginRemoveRows(QModelIndex(), row, row + count - 1);

  std::vector<zone>::iterator first = zones.begin() + row;

  zones.erase(first, first + count);

  endRemoveRows();
  return true;
}

QVariant ZoneTableModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.row() >= (int) zones.size())
    return QVariant();

  if (index.column() >= NUM_ZONE_ATTRS)
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (index.column()) {
      case 0:
        return zones[index.row()].name;
        break;
      case 1:
        return zones[index.row()].volume;
        break;
    }
  }

  return QVariant();
}
    
bool ZoneTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    switch (index.column()) {
      case 0:
        zones[index.row()].name = value.toString();
        break;
      case 1:
        zones[index.row()].volume = value.toString();
        std::cout << "update_zone:" << index.row() << ",amp," << value.toString().toStdString() << std::endl;
        break;
    }
    emit dataChanged(index, index);
    return true;
  }
  return false;
}

void InputThread::run() {
  //char input[256];
  std::string input;

  while (true) {
    std::getline(std::cin, input);
    if (std::cin.eof())
      break;

    if (!input.compare("show:1"))
      emit receivedShow();
    else if (!input.compare("show:0"))
      emit receivedHide();
    else if (!input.compare(0, 9, "add_zone:")) {
      zone z;
      std::istringstream sin(input.substr(9));
      std::string field;
      std::getline(sin, field, ',');
      z.name = QString(field.c_str());
      std::getline(sin, field, ',');
      z.volume = QString(field.c_str());

      emit receivedAddZone(z);
    }
  }
}

SamplerUI::SamplerUI() {
  QVBoxLayout* v_layout = new QVBoxLayout;

  QTableView* view = new QTableView;
  view->setModel(&zone_model);

  v_layout->addWidget(view);

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
  QModelIndex index = zone_model.index(i, 0);
  // setData commands here are dumb
  // because they write update_zone message
  // but in addNewZone case, zone change came from plugin! redundant
  zone_model.setData(index, z.name, Qt::EditRole);
  index = zone_model.index(i, 1);
  zone_model.setData(index, z.volume, Qt::EditRole);
}
