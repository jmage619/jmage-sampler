#include <iostream>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <QtWidgets>
#include <unistd.h>

#include "jm-sampler-ui.h"
#include "components.h"

/************
*
* ZoneTableView
*
**/

void ZoneTableView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
      QModelIndex index = indexAt(event->pos());
      if (index.isValid())
        edit(index);
  }
  QTableView::mousePressEvent(event);
}

/************
*
* ZoneTableDelegate
*
**/

QWidget* ZoneTableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {

  switch (index.column()) {
    case JM_ZONE_AMP: {
      DragBox* dbox = new DragBox(151, -144, 6, parent);

      // update model immediately on text change
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      // force editor close when release dragbox
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);

      //std::cout << "editor created\n";
      return dbox;
    }
    default:
      return QStyledItemDelegate::createEditor(parent, option, index);
  }
}

void ZoneTableDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem& option, const QModelIndex& index) const {
  switch (index.column()) {
    case JM_ZONE_AMP:
      static_cast<DragBox*>(editor)->setGeometry(option.rect); // have to cast because setGeometry isn't virtual
      break;
    default:
      QStyledItemDelegate::updateEditorGeometry(editor, option, index);
  }

  //std::cout << "menu popup and editor geometry updated\n";
}

void ZoneTableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  //std::cout << "begin editor update\n";
  switch (index.column()) {
    case JM_ZONE_AMP: {
      DragBox* dbox = static_cast<DragBox*>(editor);

      double val = index.model()->data(index, Qt::EditRole).toDouble();

      dbox->setValue(val);
      break;
    }
    default:
      QStyledItemDelegate::setEditorData(editor, index);
  }
  //std::cout << "editor updated\n";
}

void ZoneTableDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
    const QModelIndex& index) const {
  //std::cout << "begin model update\n";

  switch (index.column()) {
    case JM_ZONE_AMP: {
      DragBox* dbox = static_cast<DragBox*>(editor);

      model->setData(index, dbox->value(), Qt::EditRole);
      break;
    }
    default:
      QStyledItemDelegate::setModelData(editor, model, index);
  }
  //std::cout << "model updated\n";
}

void ZoneTableDelegate::updateData() {
  QWidget* editor = static_cast<QWidget*>(sender());
  emit commitData(editor);
}

void ZoneTableDelegate::forceClose() {
  QWidget* editor = static_cast<QWidget*>(sender());
  emit closeEditor(editor);
  //std::cout << "menu action triggered; commit and close\n";
}

/************
*
* ZoneTableModel
*
**/

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

  return QAbstractItemModel::flags(index) | (index.column() == JM_ZONE_LONG_TAIL ? Qt::ItemIsUserCheckable: Qt::ItemIsEditable);
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
        case JM_ZONE_LOOP_MODE:
          return "Loop";
        case JM_ZONE_CROSSFADE:
          return "CF";
        case JM_ZONE_GROUP:
          return "Group";
        case JM_ZONE_OFF_GROUP:
          return "Off Group";
        case JM_ZONE_ATTACK:
          return "Attack";
        case JM_ZONE_HOLD:
          return "Hold";
        case JM_ZONE_DECAY:
          return "Decay";
        case JM_ZONE_SUSTAIN:
          return "Sustain";
        case JM_ZONE_RELEASE:
          return "Release";
        case JM_ZONE_LONG_TAIL:
          return "20s Tail";
        case JM_ZONE_PATH:
          return "Path";
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

  if (role == Qt::CheckStateRole) {
    switch (index.column()) {
      case JM_ZONE_LONG_TAIL:
        return zones[index.row()].long_tail;
    }
  }
  else if (role == Qt::DisplayRole || role == Qt::EditRole) {
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
      case JM_ZONE_LOOP_MODE:
        return zones[index.row()].loop_mode;
      case JM_ZONE_CROSSFADE:
        return zones[index.row()].crossfade;
      case JM_ZONE_GROUP:
        return zones[index.row()].group;
      case JM_ZONE_OFF_GROUP:
        return zones[index.row()].off_group;
      case JM_ZONE_ATTACK:
        return zones[index.row()].attack;
      case JM_ZONE_HOLD:
        return zones[index.row()].hold;
      case JM_ZONE_DECAY:
        return zones[index.row()].decay;
      case JM_ZONE_SUSTAIN:
        return zones[index.row()].sustain;
      case JM_ZONE_RELEASE:
        return zones[index.row()].release;
      case JM_ZONE_PATH:
        return zones[index.row()].path;
    }
  }

  return QVariant();
}
    
bool ZoneTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::CheckStateRole) {
    switch (index.column()) {
      // long tail special case; used for UI only, don't send a message out
      case JM_ZONE_LONG_TAIL:
        zones[index.row()].long_tail = value.toInt();
        // we will likely want to modify decay and release max times here
        break;
    }
    emit dataChanged(index, index);
    return true;
  }
  else if (index.isValid() && role == Qt::EditRole) {
    std::cout << "update_zone:" << index.row() << "," << index.column() << ",";
    switch (index.column()) {
      case JM_ZONE_NAME:
        zones[index.row()].name = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_AMP:
        zones[index.row()].amp = value.toDouble();
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
      case JM_ZONE_LOOP_MODE:
        zones[index.row()].loop_mode = value.toString();
        if (value.toString() == "off")
          std::cout << LOOP_OFF;
        else if (value.toString() == "on")
          std::cout << LOOP_CONTINUOUS;
        else if (value.toString() == "one shot")
          std::cout << LOOP_ONE_SHOT;
        break;
      case JM_ZONE_CROSSFADE:
        zones[index.row()].crossfade = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_GROUP:
        zones[index.row()].group = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_OFF_GROUP:
        zones[index.row()].off_group = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_ATTACK:
        zones[index.row()].attack = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_HOLD:
        zones[index.row()].hold = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_DECAY:
        zones[index.row()].decay = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_SUSTAIN:
        zones[index.row()].sustain = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_RELEASE:
        zones[index.row()].release = value.toString();
        std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_PATH:
        zones[index.row()].path = value.toString();
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

    if (input == "show:1")
      emit receivedShow();
    else if (input == "show:0")
      emit receivedHide();
    else if (!input.compare(0, 9, "add_zone:")) {
      zone z;
      std::istringstream sin(input.substr(9));
      std::string field;
      std::getline(sin, field, ',');
      z.name = field.c_str();
      std::getline(sin, field, ',');
      z.amp = atof(field.c_str());
      std::getline(sin, field, ',');
      z.origin = field.c_str();
      std::getline(sin, field, ',');
      z.low_key = field.c_str();
      std::getline(sin, field, ',');
      z.high_key = field.c_str();
      std::getline(sin, field, ',');
      z.low_vel = field.c_str();
      std::getline(sin, field, ',');
      z.high_vel = field.c_str();
      std::getline(sin, field, ',');
      z.pitch = field.c_str();
      std::getline(sin, field, ',');
      z.start = field.c_str();
      std::getline(sin, field, ',');
      z.left = field.c_str();
      std::getline(sin, field, ',');
      z.right = field.c_str();

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
      z.crossfade = field.c_str();
      std::getline(sin, field, ',');
      z.group = field.c_str();
      std::getline(sin, field, ',');
      z.off_group = field.c_str();
      std::getline(sin, field, ',');
      z.attack = field.c_str();
      std::getline(sin, field, ',');
      z.hold = field.c_str();
      std::getline(sin, field, ',');
      z.decay = field.c_str();
      std::getline(sin, field, ',');
      z.sustain = field.c_str();
      std::getline(sin, field, ',');
      z.release = field.c_str();

      z.long_tail = atof(z.decay.toStdString().c_str()) > 2.0 || atof(z.release.toStdString().c_str()) > 2.0 ? Qt::Checked: Qt::Unchecked;

      std::getline(sin, field, ',');
      z.path = field.c_str();

      emit receivedAddZone(z);
    }
  }
}

SamplerUI::SamplerUI() {
  QVBoxLayout* v_layout = new QVBoxLayout;

  ZoneTableView* view = new ZoneTableView;
  view->setModel(&zone_model);
  view->setItemDelegate(&delegate);

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
