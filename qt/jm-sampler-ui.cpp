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

QVariant ZoneTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case JM_ZONE_NAME:
          return "Name";
        case JM_ZONE_AMP:
          return "Vol (db)";
        case JM_ZONE_ORIGIN:
          return "Origin";
        case JM_ZONE_LOW_KEY:
          return "Lower";
        case JM_ZONE_HIGH_KEY:
          return "Upper";
        case JM_ZONE_LOW_VEL:
          return "Lo Vel";
        case JM_ZONE_HIGH_VEL:
          return "Hi Vel";
        case JM_ZONE_PITCH:
          return "Pitch";
        case JM_ZONE_START:
          return "Start";
        case JM_ZONE_LEFT:
          return "Left";
        case JM_ZONE_RIGHT:
          return "Right";
        default:
          return section;
      }
    }
    return QVariant();
  }
  else
    return QAbstractItemModel::headerData(section, orientation, role);
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
      case JM_ZONE_NAME:
        return zones[index.row()].name;
      case JM_ZONE_AMP:
        return zones[index.row()].amp;
      case JM_ZONE_ORIGIN:
        return zones[index.row()].origin;
      case JM_ZONE_LOW_KEY:
        return zones[index.row()].low_key;
      case JM_ZONE_HIGH_KEY:
        return zones[index.row()].high_key;
      case JM_ZONE_LOW_VEL:
        return zones[index.row()].low_vel;
      case JM_ZONE_HIGH_VEL:
        return zones[index.row()].high_vel;
      case JM_ZONE_PITCH:
        return zones[index.row()].pitch;
      case JM_ZONE_START:
        return zones[index.row()].start;
      case JM_ZONE_LEFT:
        return zones[index.row()].left;
      case JM_ZONE_RIGHT:
        return zones[index.row()].right;
    }
  }

  return QVariant();
}
    
bool ZoneTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::EditRole) {
    std::cout << "update_zone:" << index.row() << "," << index.column() << ",";
    switch (index.column()) {
      case JM_ZONE_NAME:
        zones[index.row()].name = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_AMP:
        zones[index.row()].amp = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_ORIGIN:
        zones[index.row()].origin = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_LOW_KEY:
        zones[index.row()].low_key = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_HIGH_KEY:
        zones[index.row()].high_key = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_LOW_VEL:
        zones[index.row()].low_vel = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_HIGH_VEL:
        zones[index.row()].high_vel = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_PITCH:
        zones[index.row()].pitch = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_START:
        zones[index.row()].start = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_LEFT:
        zones[index.row()].left = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_RIGHT:
        zones[index.row()].right = value.toString();
        std::cout << value.toString().toStdString();
        break;
    }
    std::cout << std::endl;

    emit dataChanged(index, index);
    return true;
  }
  return false;
}

void ZoneTableModel::setZone(int row, const zone& z) {
  zones[row] = z;
  emit dataChanged(index(row, 0), index(row, NUM_ZONE_ATTRS - 1));
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
      z.amp = QString(field.c_str());
      std::getline(sin, field, ',');
      z.origin = QString(field.c_str());
      std::getline(sin, field, ',');
      z.low_key = QString(field.c_str());
      std::getline(sin, field, ',');
      z.high_key = QString(field.c_str());
      std::getline(sin, field, ',');
      z.low_vel = QString(field.c_str());
      std::getline(sin, field, ',');
      z.high_vel = QString(field.c_str());
      std::getline(sin, field, ',');
      z.pitch = QString(field.c_str());
      std::getline(sin, field, ',');
      z.start = QString(field.c_str());
      std::getline(sin, field, ',');
      z.left = QString(field.c_str());
      std::getline(sin, field, ',');
      z.right = QString(field.c_str());

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
  zone_model.setZone(i, z);
}
