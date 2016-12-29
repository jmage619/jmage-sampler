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
* SingleClickView
*
**/

void SingleClickView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
      QModelIndex index = indexAt(event->pos());
      if (index.isValid()) {
        setCurrentIndex(index);
        if (index.flags() & Qt::ItemIsEditable)
          edit(index);
      }
  }
  QTableView::mousePressEvent(event);
}

/************
*
* ZoneTableView
*
**/

void ZoneTableView::updateFrozenTableGeometry() {
  frozen_view->setGeometry(frameWidth(), frameWidth(),
    verticalHeader()->width() + columnWidth(0) + columnWidth(1),
    viewport()->height()+horizontalHeader()->height());
}

void ZoneTableView::init() {
  frozen_view->setModel(model());
  ZoneTableDelegate* frozenDelegate = new ZoneTableDelegate(frozen_view);
  frozen_view->setItemDelegate(frozenDelegate);
  frozen_view->setFrameStyle(QFrame::NoFrame);
  frozen_view->setFocusPolicy(Qt::NoFocus);

  viewport()->stackUnder(frozen_view);

  frozen_view->setSelectionModel(selectionModel());
  for (int col = 2; col < model()->columnCount(); ++col)
        frozen_view->setColumnHidden(col, true);

  frozen_view->setColumnWidth(0, columnWidth(0));
  frozen_view->setColumnWidth(1, columnWidth(1));

  frozen_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  frozen_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  frozen_view->show();

  updateFrozenTableGeometry();

  setHorizontalScrollMode(ScrollPerPixel);
  setVerticalScrollMode(ScrollPerPixel);
  frozen_view->setVerticalScrollMode(ScrollPerPixel);
}

ZoneTableView::ZoneTableView(QAbstractItemModel* model) {
  setModel(model);
  ZoneTableDelegate* delegate = new ZoneTableDelegate(this);
  setItemDelegate(delegate);

  frozen_view = new SingleClickView(this);

  init();

  //connect the headers and scrollbars of both tableviews together
  connect(horizontalHeader(),&QHeaderView::sectionResized, this,
    &ZoneTableView::updateFrozenSectionWidth);
  connect(frozen_view->horizontalHeader(),&QHeaderView::sectionResized, this,
    &ZoneTableView::updateSectionWidth);
  connect(frozen_view->verticalHeader(),&QHeaderView::sectionResized, this,
    &ZoneTableView::updateSectionHeight);

  connect(frozen_view->verticalScrollBar(), &QAbstractSlider::valueChanged,
    verticalScrollBar(), &QAbstractSlider::setValue);
  connect(verticalScrollBar(), &QAbstractSlider::valueChanged,
    frozen_view->verticalScrollBar(), &QAbstractSlider::setValue);
}

void ZoneTableView::setSelectionMode(QAbstractItemView::SelectionMode mode) {
  QAbstractItemView::setSelectionMode(mode);
  frozen_view->setSelectionMode(mode);
}

void ZoneTableView::resizeEvent(QResizeEvent* event) {
  QTableView::resizeEvent(event);
  updateFrozenTableGeometry();
}

QModelIndex ZoneTableView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) {
  QModelIndex current = QTableView::moveCursor(cursorAction, modifiers);

  if (cursorAction == MoveLeft && current.column() > 1
      && visualRect(current).topLeft().x() < frozen_view->columnWidth(0) + frozen_view->columnWidth(1)) {
    const int newValue = horizontalScrollBar()->value() + visualRect(current).topLeft().x()
                         - (frozen_view->columnWidth(0) + frozen_view->columnWidth(1));
    horizontalScrollBar()->setValue(newValue);
  }
  return current;
}

void ZoneTableView::scrollTo(const QModelIndex& index, ScrollHint hint) {
  if (index.column() > 1)
    QTableView::scrollTo(index, hint);
}

void ZoneTableView::updateSectionWidth(int logicalIndex, int /* oldSize */, int newSize) {
  setColumnWidth(logicalIndex, newSize);
}

void ZoneTableView::updateFrozenSectionWidth(int logicalIndex, int /* oldSize */, int newSize) {
  switch (logicalIndex) {
    case 0:
    case 1:
      frozen_view->setColumnWidth(logicalIndex, newSize);
      updateFrozenTableGeometry();
      break;
  }
}

void ZoneTableView::updateSectionHeight(int logicalIndex, int /* oldSize */, int newSize) {
  setRowHeight(logicalIndex, newSize);
}

/************
*
* ZoneTableDelegate
*
**/

QWidget* ZoneTableDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {

  DragBox* dbox;
  QComboBox* combo;
  switch (index.column()) {
    case JM_ZONE_AMP:
      dbox = new DragBox(parent, -144, 6, 151);
      // update model immediately on text change
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      // force editor close when release dragbox
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);

      //std::cout << "editor created\n";
      return dbox;
    case JM_ZONE_ORIGIN:
    case JM_ZONE_LOW_KEY:
    case JM_ZONE_HIGH_KEY: {
      NotePopup* popup = new NotePopup(parent);
      connect(popup, &NotePopup::selected, this, &ZoneTableDelegate::commitAndCloseEditor);
      return popup;
    }
    case JM_ZONE_LOW_VEL:
    case JM_ZONE_HIGH_VEL:
      dbox = new DragBox(parent, 0, 127, 128);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case JM_ZONE_PITCH:
      dbox = new DragBox(parent, -.5, .5);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case JM_ZONE_START:
    case JM_ZONE_LEFT:
    case JM_ZONE_RIGHT:
      dbox = new DragBox(parent, 0.0, index.data(JM_MAX_ROLE).toDouble());
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case JM_ZONE_LOOP_MODE:
      combo = new QComboBox(parent);
      combo->addItem(tr("off"));
      combo->addItem(tr("on"));
      combo->addItem(tr("one shot"));
      // activated better than index changed for case where user clicks same
      // still get a combo box if click outside border or hit escape..
      connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ZoneTableDelegate::commitAndCloseEditor);
      return combo;
    case JM_ZONE_GROUP:
    case JM_ZONE_OFF_GROUP:
      combo = new QComboBox(parent);
      combo->addItem(tr("none"));
      for (int i = 1; i <= 16; ++i)
        combo->addItem(QString::number(i));

      connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ZoneTableDelegate::commitAndCloseEditor);
      return combo;
    case JM_ZONE_CROSSFADE:
      dbox = new DragBox(parent, 0, 1000);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case JM_ZONE_ATTACK:
    case JM_ZONE_HOLD:
      dbox = new DragBox(parent, 0.0, 2.0);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case JM_ZONE_DECAY:
    case JM_ZONE_RELEASE: {
      const QAbstractItemModel* model = index.model();
      QModelIndex i = model->index(index.row(), JM_ZONE_LONG_TAIL);
      if (i.data(Qt::CheckStateRole).toInt() == Qt::Checked)
        dbox = new DragBox(parent, 0.0, 20.0);
      else
        dbox = new DragBox(parent, 0.0, 2.0);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    }
    case JM_ZONE_SUSTAIN:
      dbox = new DragBox(parent, 0.0, 1.0);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    default:
      return QStyledItemDelegate::createEditor(parent, option, index);
  }
}

void ZoneTableDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem& option, const QModelIndex& index) const {
  switch (index.column()) {
    case JM_ZONE_AMP:
    case JM_ZONE_LOW_VEL:
    case JM_ZONE_HIGH_VEL:
    case JM_ZONE_PITCH:
    case JM_ZONE_START:
    case JM_ZONE_LEFT:
    case JM_ZONE_RIGHT:
    case JM_ZONE_CROSSFADE:
    case JM_ZONE_ATTACK:
    case JM_ZONE_HOLD:
    case JM_ZONE_DECAY:
    case JM_ZONE_SUSTAIN:
    case JM_ZONE_RELEASE: {
      DragBox* dbox = static_cast<DragBox*>(editor);
      dbox->setGeometry(option.rect); // have to cast because setGeometry isn't virtual
      dbox->showPopup();
      break;
    }
    case JM_ZONE_ORIGIN:
    case JM_ZONE_LOW_KEY:
    case JM_ZONE_HIGH_KEY: {
      NotePopup* popup = static_cast<NotePopup*>(editor);
      popup->setGeometry(option.rect);
      popup->showPopup();
      break;
    }
    case JM_ZONE_LOOP_MODE:
    case JM_ZONE_GROUP:
    case JM_ZONE_OFF_GROUP: {
      QComboBox* combo = static_cast<QComboBox*>(editor);
      // base new y off centers since current cell may have been resized
      int y = option.rect.y() + (option.rect.height() - combo->height()) / 2;
      // set width to grid cell
      combo->setGeometry(option.rect.x(), y, option.rect.width(), combo->height());
      break;
    }
    default:
      QStyledItemDelegate::updateEditorGeometry(editor, option, index);
  }

  //std::cout << "menu popup and editor geometry updated\n";
}

void ZoneTableDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const {
  //std::cout << "begin editor update\n";
  
  switch (index.column()) {
    case JM_ZONE_AMP:
    case JM_ZONE_LOW_VEL:
    case JM_ZONE_HIGH_VEL:
    case JM_ZONE_PITCH:
    case JM_ZONE_START:
    case JM_ZONE_LEFT:
    case JM_ZONE_RIGHT:
    case JM_ZONE_CROSSFADE:
    case JM_ZONE_ATTACK:
    case JM_ZONE_HOLD:
    case JM_ZONE_DECAY:
    case JM_ZONE_SUSTAIN:
    case JM_ZONE_RELEASE: {
      DragBox* dbox = static_cast<DragBox*>(editor);

      double val = index.data(Qt::EditRole).toDouble();

      dbox->setValue(val);
      break;
    }
    case JM_ZONE_ORIGIN:
    case JM_ZONE_LOW_KEY:
    case JM_ZONE_HIGH_KEY: {
      NotePopup* popup = static_cast<NotePopup*>(editor);
      popup->setCurrentText(index.data(Qt::EditRole).toString());
      break;
    }
    case JM_ZONE_LOOP_MODE:
    case JM_ZONE_GROUP:
    case JM_ZONE_OFF_GROUP: {
      QComboBox* combo = static_cast<QComboBox*>(editor);
      combo->setCurrentText(index.data(Qt::EditRole).toString());
      // must show popup here instead of update geometry because it
      // must be called after set cur text to properly place menu
      combo->showPopup();
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
    case JM_ZONE_AMP:
    case JM_ZONE_LOW_VEL:
    case JM_ZONE_HIGH_VEL:
    case JM_ZONE_PITCH:
    case JM_ZONE_START:
    case JM_ZONE_LEFT:
    case JM_ZONE_RIGHT:
    case JM_ZONE_CROSSFADE:
    case JM_ZONE_ATTACK:
    case JM_ZONE_HOLD:
    case JM_ZONE_DECAY:
    case JM_ZONE_SUSTAIN:
    case JM_ZONE_RELEASE: {
      DragBox* dbox = static_cast<DragBox*>(editor);

      model->setData(index, dbox->value(), Qt::EditRole);
      break;
    }
    case JM_ZONE_ORIGIN:
    case JM_ZONE_LOW_KEY:
    case JM_ZONE_HIGH_KEY: {
      NotePopup* popup = static_cast<NotePopup*>(editor);
      model->setData(index, popup->currentText(), Qt::EditRole);
      break;
    }
    case JM_ZONE_LOOP_MODE:
    case JM_ZONE_GROUP:
    case JM_ZONE_OFF_GROUP: {
      QComboBox* combo = static_cast<QComboBox*>(editor);
      model->setData(index, combo->currentText(), Qt::EditRole);
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

void ZoneTableDelegate::commitAndCloseEditor() {
  QWidget* editor = static_cast<QWidget*>(sender());
  emit commitData(editor);
  emit closeEditor(editor);
}

// some helper functions
namespace {
  QString note_to_string(int note) {
    // first octave is -1, so subtract to translate to it
    int octave = note / 12 - 1;
    int pos = note % 12;

    switch (pos) {
      case 0:
        return QString::number(octave) + "C";
      case 1:
        return QString::number(octave) + "C#";
      case 2:
        return QString::number(octave) + "D";
      case 3:
        return QString::number(octave) + "D#";
      case 4:
        return QString::number(octave) + "E";
      case 5:
        return QString::number(octave) + "F";
      case 6:
        return QString::number(octave) + "F#";
      case 7:
        return QString::number(octave) + "G";
      case 8:
        return QString::number(octave) + "G#";
      case 9:
        return QString::number(octave) + "A";
      case 10:
        return QString::number(octave) + "A#";
      case 11:
        return QString::number(octave) + "B";
    }

    return "";
  }

  int string_to_note(const QString& str) {
    // first octave is -1, add 1 to make it a valid index
    int octave = str.startsWith("-1") ? 0: str[0].digitValue() + 1;

    // if octave -1, extra character to account for negative sign
    QString note = octave == 0 ? str.mid(2): str.mid(1);

    if (note == "C")
      return octave * 12 + 0;
    if (note == "C#")
      return octave * 12 + 1;
    if (note == "D")
      return octave * 12 + 2;
    if (note == "D#")
      return octave * 12 + 3;
    if (note == "E")
      return octave * 12 + 4;
    if (note == "F")
      return octave * 12 + 5;
    if (note == "F#")
      return octave * 12 + 6;
    if (note == "G")
      return octave * 12 + 7;
    if (note == "G#")
      return octave * 12 + 8;
    if (note == "A")
      return octave * 12 + 9;
    if (note == "A#")
      return octave * 12 + 10;
    if (note == "B")
      return octave * 12 + 11;

    return -1;
  }
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
  else if (role == JM_MAX_ROLE) {
    switch (index.column()) {
      case JM_ZONE_START:
      case JM_ZONE_LEFT:
      case JM_ZONE_RIGHT:
        return zones[index.row()].wave_length;
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
        std::cout << value.toDouble();
        break;
      case JM_ZONE_ORIGIN:
        zones[index.row()].origin = value.toString();
        std::cout << string_to_note(value.toString());
        break;
      case JM_ZONE_LOW_KEY:
        zones[index.row()].low_key = value.toString();
        std::cout << string_to_note(value.toString());
        break;
      case JM_ZONE_HIGH_KEY:
        zones[index.row()].high_key = value.toString();
        std::cout << string_to_note(value.toString());
        break;
      case JM_ZONE_LOW_VEL:
        zones[index.row()].low_vel = value.toDouble();
        std::cout << value.toInt(); // convert to int for output
        break;
      case JM_ZONE_HIGH_VEL:
        zones[index.row()].high_vel = value.toDouble();
        std::cout << value.toInt();
        break;
      case JM_ZONE_PITCH:
        zones[index.row()].pitch = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_START:
        zones[index.row()].start = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_LEFT:
        zones[index.row()].left = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_RIGHT:
        zones[index.row()].right = value.toDouble();
        std::cout << value.toDouble();
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
        zones[index.row()].crossfade = value.toDouble();
        std::cout << value.toInt();
        break;
      case JM_ZONE_GROUP:
        zones[index.row()].group = value.toString();
        if (value.toString() == "none")
          std::cout << "0";
        else
          std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_OFF_GROUP:
        zones[index.row()].off_group = value.toString();
        if (value.toString() == "none")
          std::cout << "0";
        else
          std::cout << value.toString().toStdString();
        break;
      case JM_ZONE_ATTACK:
        zones[index.row()].attack = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_HOLD:
        zones[index.row()].hold = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_DECAY:
        zones[index.row()].decay = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_SUSTAIN:
        zones[index.row()].sustain = value.toDouble();
        std::cout << value.toDouble();
        break;
      case JM_ZONE_RELEASE:
        zones[index.row()].release = value.toDouble();
        std::cout << value.toDouble();
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
