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
#include <cstring>
#include <cmath>
#include <QtWidgets>
#include <libgen.h>

#include <lib/zone.h>
#include "components.h"
#include "zonegrid.h"

// some helper functions
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

/************
*
* MouseHandleView
*
**/

void MouseHandleView::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
      setCurrentIndex(index);
      if (index.flags() & Qt::ItemIsEditable)
        edit(index);
    }
  }
  else if (event->button() == Qt::MidButton) {
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
      switch (index.column()) {
        case jm::ZONE_AMP:
        case jm::ZONE_PITCH:
          model()->setData(index, 0.0, Qt::EditRole);
          emit userUpdate();
          break;
      }
    }
  }
  QTableView::mousePressEvent(event);
}

void MouseHandleView::wheelEvent(QWheelEvent *event) {
  QModelIndex index = indexAt(event->pos());

  Control* cont = NULL;

  if (index.isValid()) {
    switch (index.column()) {
      case jm::ZONE_AMP:
        cont = new VolumeControl;
        break;
      case jm::ZONE_LOW_VEL:
      case jm::ZONE_HIGH_VEL:
        cont = new LinearControl(0, 127, 128);
        break;
      case jm::ZONE_PITCH:
        cont = new LinearControl(-.5, .5);
        break;
      case jm::ZONE_START:
      case jm::ZONE_LEFT:
      case jm::ZONE_RIGHT:
        cont = new LinearControl(0, index.data(MAX_ROLE).toDouble());
        break;
      case jm::ZONE_CROSSFADE:
        cont = new LinearControl(0, 1000);
        break;
      case jm::ZONE_ATTACK:
      case jm::ZONE_HOLD:
        cont = new LinearControl(0, 2);
        break;
      case jm::ZONE_DECAY:
      case jm::ZONE_RELEASE: {
        QModelIndex i = model()->index(index.row(), jm::ZONE_LONG_TAIL);
        if (i.data(Qt::CheckStateRole).toInt() == Qt::Checked)
          cont = new LinearControl(0, 20);
        else
          cont = new LinearControl(0, 2);
        break;
      }
      case jm::ZONE_SUSTAIN:
        cont = new LinearControl(0, 1);
        break;
    }
  }

  if (cont != NULL) {
    cont->setValue(index.data().toDouble());
    // mouse wheel clicks are 120 at a time
    cont->increase(event->angleDelta().y() / 120);
    model()->setData(index, cont->value(), Qt::EditRole);
    delete cont;
    emit userUpdate();
  }

  event->accept();
}

/************
*
* ZoneTableView
*
**/

void ZoneTableView::updateFrozenTableGeometry() {
  frozen_view->setGeometry(frameWidth(), frameWidth(),
    verticalHeader()->width() + columnWidth(0) + columnWidth(1) + columnWidth(2) + columnWidth(3),
    viewport()->height()+horizontalHeader()->height());

  //std::cout << "     hor w: " << horizontalHeader()->width() << ";      hor x: " << horizontalHeader()->x() << std::endl;
  //std::cout << "froz hor w: " << frozen_view->horizontalHeader()->width() << "; froz hor x: " << frozen_view->horizontalHeader()->x() << std::endl;
}

void ZoneTableView::init() {
  frozen_view->setModel(model());
  ZoneTableDelegate* frozenDelegate = new ZoneTableDelegate(frozen_view, 1);
  connect(frozenDelegate, &QAbstractItemDelegate::commitData, this, &MouseHandleView::userUpdate);
  frozen_view->setItemDelegate(frozenDelegate);
  frozen_view->setFrameStyle(QFrame::NoFrame);
  frozen_view->setFocusPolicy(Qt::NoFocus);

  viewport()->stackUnder(frozen_view);

  frozen_view->setSelectionModel(selectionModel());
  for (int col = 4; col < model()->columnCount(); ++col)
        frozen_view->setColumnHidden(col, true);

  frozen_view->setColumnWidth(0, columnWidth(0));
  frozen_view->setColumnWidth(1, columnWidth(1));
  frozen_view->setColumnWidth(2, columnWidth(2));
  frozen_view->setColumnWidth(3, columnWidth(3));

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
  ZoneTableDelegate* p3_delegate = new ZoneTableDelegate(this, 3);
  setItemDelegateForColumn(jm::ZONE_START, p3_delegate);
  setItemDelegateForColumn(jm::ZONE_LEFT, p3_delegate);
  setItemDelegateForColumn(jm::ZONE_RIGHT, p3_delegate);

  connect(delegate, &QAbstractItemDelegate::commitData, this, &MouseHandleView::userUpdate);
  connect(p3_delegate, &QAbstractItemDelegate::commitData, this, &MouseHandleView::userUpdate);

  frozen_view = new MouseHandleView(this);

  init();

  connect(frozen_view, &MouseHandleView::userUpdate, this, &MouseHandleView::userUpdate);

  frozen_view->verticalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(frozen_view->verticalHeader(), &QWidget::customContextMenuRequested, this, &ZoneTableView::handleVertHeaderClick);

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

  // horizontal header moves right when zones are present and back to 0 when empty
  // trigger a updateFrozenTableGeometry when this happens
  horizontalHeader()->installEventFilter(this);

  setColumnWidth(jm::ZONE_AMP, 65);
  setColumnWidth(jm::ZONE_MUTE, 30);
  setColumnWidth(jm::ZONE_SOLO, 30);
  setColumnWidth(jm::ZONE_ORIGIN, 55);
  setColumnWidth(jm::ZONE_LOW_KEY, 55);
  setColumnWidth(jm::ZONE_HIGH_KEY, 55);
  setColumnWidth(jm::ZONE_LOW_VEL, 55);
  setColumnWidth(jm::ZONE_HIGH_VEL, 55);
  setColumnWidth(jm::ZONE_PITCH, 55);
  setColumnWidth(jm::ZONE_START, 65);
  setColumnWidth(jm::ZONE_LEFT, 65);
  setColumnWidth(jm::ZONE_RIGHT, 65);
  setColumnWidth(jm::ZONE_LOOP_MODE, 65);
  setColumnWidth(jm::ZONE_CROSSFADE, 55);
  setColumnWidth(jm::ZONE_GROUP, 65);
  setColumnWidth(jm::ZONE_OFF_GROUP, 65);
  setColumnWidth(jm::ZONE_ATTACK, 65);
  setColumnWidth(jm::ZONE_HOLD, 65);
  setColumnWidth(jm::ZONE_DECAY, 65);
  setColumnWidth(jm::ZONE_SUSTAIN, 65);
  setColumnWidth(jm::ZONE_RELEASE, 65);
  setColumnWidth(jm::ZONE_LONG_TAIL, 30);
}

bool ZoneTableView::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::Move)
    updateFrozenTableGeometry();

  return QObject::eventFilter(obj, event);
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

  if (cursorAction == MoveLeft && current.column() > 3
      && visualRect(current).topLeft().x() < frozen_view->columnWidth(0) + frozen_view->columnWidth(1)
      + frozen_view->columnWidth(2) + frozen_view->columnWidth(3)) {
    const int newValue = horizontalScrollBar()->value() + visualRect(current).topLeft().x()
      - (frozen_view->columnWidth(0) + frozen_view->columnWidth(1) +
      frozen_view->columnWidth(2) + frozen_view->columnWidth(3));
    horizontalScrollBar()->setValue(newValue);
  }
  return current;
}

void ZoneTableView::scrollTo(const QModelIndex& index, ScrollHint hint) {
  if (index.column() > 3)
    QTableView::scrollTo(index, hint);
}

void ZoneTableView::updateSectionWidth(int logicalIndex, int /* oldSize */, int newSize) {
  setColumnWidth(logicalIndex, newSize);
}

void ZoneTableView::updateFrozenSectionWidth(int logicalIndex, int /* oldSize */, int newSize) {
  switch (logicalIndex) {
    case 0:
    case 1:
    case 2:
    case 3:
      frozen_view->setColumnWidth(logicalIndex, newSize);
      updateFrozenTableGeometry();
      break;
  }
}

void ZoneTableView::updateSectionHeight(int logicalIndex, int /* oldSize */, int newSize) {
  setRowHeight(logicalIndex, newSize);
}

void ZoneTableView::handleVertHeaderClick(const QPoint& pos) {
  QMenu menu;
  menu.addAction(tr("delete"));
  menu.addAction(tr("new zone before"));
  menu.addAction(tr("duplicate"));
  QAction* action = menu.exec(verticalHeader()->mapToGlobal(pos));
  if (action != 0) {
    if (action->text() == "delete") {
      std::cout << "remove_zone:" << verticalHeader()->logicalIndexAt(pos) << std::endl;
      emit userUpdate();
    }
    else if (action->text() == "new zone before") {
      QString path = QFileDialog::getOpenFileName(this, tr("Open a fucking WAV already!!"), "", tr("Sound Files (*.wav *.WAV *.aiff *.flac)"));
      if (!path.isNull()) {
        std::cout << "add_zone:" << verticalHeader()->logicalIndexAt(pos) << "," << path.toStdString() << std::endl;
        emit userUpdate();
      }
    }
    else if (action->text() == "duplicate") {
      std::cout << "dup_zone:" << verticalHeader()->logicalIndexAt(pos) << std::endl;
      emit userUpdate();
    }
  }
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
    case jm::ZONE_AMP:
      dbox = new VolumeDragBox(parent);
      // update model immediately on text change
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      // force editor close when release dragbox
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);

      //std::cout << "editor created\n";
      return dbox;
    case jm::ZONE_ORIGIN:
    case jm::ZONE_LOW_KEY:
    case jm::ZONE_HIGH_KEY: {
      NotePopup* popup = new NotePopup(parent);
      connect(popup, &NotePopup::selected, this, &ZoneTableDelegate::commitAndCloseEditor);
      return popup;
    }
    case jm::ZONE_LOW_VEL:
    case jm::ZONE_HIGH_VEL:
      dbox = new LinearDragBox(parent, 0, 127, 128);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case jm::ZONE_PITCH:
      dbox = new LinearDragBox(parent, -.5, .5, 101, 2);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case jm::ZONE_START:
    case jm::ZONE_LEFT:
    case jm::ZONE_RIGHT:
      dbox = new LinearDragBox(parent, 0.0, index.data(MAX_ROLE).toDouble(), 101, 3);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case jm::ZONE_LOOP_MODE:
      combo = new QComboBox(parent);
      combo->addItem(tr("off"));
      combo->addItem(tr("on"));
      combo->addItem(tr("one shot"));
      // activated better than index changed for case where user clicks same
      // still get a combo box if click outside border or hit escape..
      connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ZoneTableDelegate::commitAndCloseEditor);
      return combo;
    case jm::ZONE_GROUP:
    case jm::ZONE_OFF_GROUP:
      combo = new QComboBox(parent);
      combo->addItem(tr("none"));
      for (int i = 1; i <= 16; ++i)
        combo->addItem(QString::number(i));

      connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &ZoneTableDelegate::commitAndCloseEditor);
      return combo;
    case jm::ZONE_CROSSFADE:
      dbox = new LinearDragBox(parent, 0, 1000);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case jm::ZONE_ATTACK:
    case jm::ZONE_HOLD:
      dbox = new LinearDragBox(parent, 0.0, 2.0, 101, 2);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case jm::ZONE_DECAY:
    case jm::ZONE_RELEASE: {
      const QAbstractItemModel* model = index.model();
      QModelIndex i = model->index(index.row(), jm::ZONE_LONG_TAIL);
      if (i.data(Qt::CheckStateRole).toInt() == Qt::Checked)
        dbox = new LinearDragBox(parent, 0.0, 20.0, 101, 2);
      else
        dbox = new LinearDragBox(parent, 0.0, 2.0, 101, 2);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    }
    case jm::ZONE_SUSTAIN:
      dbox = new LinearDragBox(parent, 0.0, 1.0, 101, 2);
      connect(dbox, &DragBox::dragged, this, &ZoneTableDelegate::updateData);
      connect(dbox, &DragBox::released, this, &ZoneTableDelegate::forceClose);
      return dbox;
    case jm::ZONE_PATH: {
      QFileDialog* file_dialog = new QFileDialog(parent, tr("Open a fucking WAV already!!"));
      file_dialog->setModal(true);
      file_dialog->setFileMode(QFileDialog::ExistingFile);
      file_dialog->setNameFilter(tr("Sound Files (*.wav *.WAV *.aiff *.flac)"));
      return file_dialog;
    }
    default:
      return QStyledItemDelegate::createEditor(parent, option, index);
  }
}

void ZoneTableDelegate::updateEditorGeometry(QWidget* editor,
    const QStyleOptionViewItem& option, const QModelIndex& index) const {
  switch (index.column()) {
    case jm::ZONE_AMP:
    case jm::ZONE_LOW_VEL:
    case jm::ZONE_HIGH_VEL:
    case jm::ZONE_PITCH:
    case jm::ZONE_START:
    case jm::ZONE_LEFT:
    case jm::ZONE_RIGHT:
    case jm::ZONE_CROSSFADE:
    case jm::ZONE_ATTACK:
    case jm::ZONE_HOLD:
    case jm::ZONE_DECAY:
    case jm::ZONE_SUSTAIN:
    case jm::ZONE_RELEASE: {
      DragBox* dbox = static_cast<DragBox*>(editor);
      dbox->setGeometry(option.rect); // have to cast because setGeometry isn't virtual
      dbox->showPopup();
      break;
    }
    case jm::ZONE_ORIGIN:
    case jm::ZONE_LOW_KEY:
    case jm::ZONE_HIGH_KEY: {
      NotePopup* popup = static_cast<NotePopup*>(editor);
      popup->setGeometry(option.rect);
      popup->showPopup();
      break;
    }
    case jm::ZONE_LOOP_MODE:
    case jm::ZONE_GROUP:
    case jm::ZONE_OFF_GROUP: {
      QComboBox* combo = static_cast<QComboBox*>(editor);
      // base new y off centers since current cell may have been resized
      int y = option.rect.y() + (option.rect.height() - combo->height()) / 2;
      // set width to grid cell
      combo->setGeometry(option.rect.x(), y, option.rect.width(), combo->height());
      break;
    }
    case jm::ZONE_PATH: {
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
    case jm::ZONE_AMP:
    case jm::ZONE_LOW_VEL:
    case jm::ZONE_HIGH_VEL:
    case jm::ZONE_PITCH:
    case jm::ZONE_START:
    case jm::ZONE_LEFT:
    case jm::ZONE_RIGHT:
    case jm::ZONE_CROSSFADE:
    case jm::ZONE_ATTACK:
    case jm::ZONE_HOLD:
    case jm::ZONE_DECAY:
    case jm::ZONE_SUSTAIN:
    case jm::ZONE_RELEASE: {
      DragBox* dbox = static_cast<DragBox*>(editor);

      double val = index.data(Qt::EditRole).toDouble();

      dbox->setValue(val);
      break;
    }
    case jm::ZONE_ORIGIN:
    case jm::ZONE_LOW_KEY:
    case jm::ZONE_HIGH_KEY: {
      NotePopup* popup = static_cast<NotePopup*>(editor);
      popup->setCurrentText(index.data(Qt::EditRole).toString());
      break;
    }
    case jm::ZONE_LOOP_MODE:
    case jm::ZONE_GROUP:
    case jm::ZONE_OFF_GROUP: {
      QComboBox* combo = static_cast<QComboBox*>(editor);
      combo->setCurrentText(index.data(Qt::EditRole).toString());
      // must show popup here instead of update geometry because it
      // must be called after set cur text to properly place menu
      combo->showPopup();
      break;
    }
    case jm::ZONE_PATH: {
      QFileDialog* file_dialog = static_cast<QFileDialog*>(editor);
      QString path = index.data(Qt::EditRole).toString();
      char* tmp_str = strdup(path.toStdString().c_str());
      file_dialog->setDirectory(dirname(tmp_str));
      free(tmp_str);
      tmp_str = strdup(path.toStdString().c_str());
      file_dialog->selectFile(basename(tmp_str));
      free(tmp_str);
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
    case jm::ZONE_AMP:
    case jm::ZONE_LOW_VEL:
    case jm::ZONE_HIGH_VEL:
    case jm::ZONE_PITCH:
    case jm::ZONE_START:
    case jm::ZONE_LEFT:
    case jm::ZONE_RIGHT:
    case jm::ZONE_CROSSFADE:
    case jm::ZONE_ATTACK:
    case jm::ZONE_HOLD:
    case jm::ZONE_DECAY:
    case jm::ZONE_SUSTAIN:
    case jm::ZONE_RELEASE: {
      DragBox* dbox = static_cast<DragBox*>(editor);

      model->setData(index, dbox->value(), Qt::EditRole);
      break;
    }
    case jm::ZONE_ORIGIN:
    case jm::ZONE_LOW_KEY:
    case jm::ZONE_HIGH_KEY: {
      NotePopup* popup = static_cast<NotePopup*>(editor);
      model->setData(index, popup->currentText(), Qt::EditRole);
      break;
    }
    case jm::ZONE_LOOP_MODE:
    case jm::ZONE_GROUP:
    case jm::ZONE_OFF_GROUP: {
      QComboBox* combo = static_cast<QComboBox*>(editor);
      model->setData(index, combo->currentText(), Qt::EditRole);
      break;
    }
    case jm::ZONE_PATH: {
      QFileDialog* file_dialog = static_cast<QFileDialog*>(editor);
      model->setData(index, file_dialog->selectedFiles()[0], Qt::EditRole);
      break;
    }
    default:
      QStyledItemDelegate::setModelData(editor, model, index);
  }
  //std::cout << "model updated\n";
}

QString ZoneTableDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  if (value.type() == QVariant::Double || (enum QMetaType::Type) value.type() == QMetaType::Float)
    return locale.toString(value.toDouble(), 'f', precision);

  return QStyledItemDelegate::displayText(value, locale);
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

  switch (index.column()) {
    case jm::ZONE_MUTE:
    case jm::ZONE_SOLO:
    case jm::ZONE_LONG_TAIL:
      return QAbstractItemModel::flags(index) | Qt::ItemIsUserCheckable;
    default:
      return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
  }
}

bool ZoneTableModel::insertRows(int row, int count, const QModelIndex&) {
  beginInsertRows(QModelIndex(), row, row + count - 1);

  std::vector<jm::zone>::iterator first = zones.begin() + row;
  zones.insert(first, count, jm::zone());

  endInsertRows();
  return true;
}

bool ZoneTableModel::removeRows(int row, int count, const QModelIndex&) {
  beginRemoveRows(QModelIndex(), row, row + count - 1);

  std::vector<jm::zone>::iterator first = zones.begin() + row;

  zones.erase(first, first + count);

  endRemoveRows();
  return true;
}

QVariant ZoneTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal) {
    if (role == Qt::DisplayRole) {
      switch (section) {
        case jm::ZONE_NAME:
          return "Name";
        case jm::ZONE_AMP:
          return "Vol (db)";
        case jm::ZONE_MUTE:
          return "M";
        case jm::ZONE_SOLO:
          return "S";
        case jm::ZONE_ORIGIN:
          return "Origin";
        case jm::ZONE_LOW_KEY:
          return "Lower";
        case jm::ZONE_HIGH_KEY:
          return "Upper";
        case jm::ZONE_LOW_VEL:
          return "Lo Vel";
        case jm::ZONE_HIGH_VEL:
          return "Hi Vel";
        case jm::ZONE_PITCH:
          return "Pitch";
        case jm::ZONE_START:
          return "Start";
        case jm::ZONE_LEFT:
          return "Left";
        case jm::ZONE_RIGHT:
          return "Right";
        case jm::ZONE_LOOP_MODE:
          return "Loop";
        case jm::ZONE_CROSSFADE:
          return "CF";
        case jm::ZONE_GROUP:
          return "Group";
        case jm::ZONE_OFF_GROUP:
          return "Off Grp";
        case jm::ZONE_ATTACK:
          return "Attack";
        case jm::ZONE_HOLD:
          return "Hold";
        case jm::ZONE_DECAY:
          return "Decay";
        case jm::ZONE_SUSTAIN:
          return "Sustain";
        case jm::ZONE_RELEASE:
          return "Release";
        case jm::ZONE_LONG_TAIL:
          return "20s";
        case jm::ZONE_PATH:
          return "File";
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
      case jm::ZONE_MUTE:
        return zones[index.row()].mute;
      case jm::ZONE_SOLO:
        return zones[index.row()].solo;
      case jm::ZONE_LONG_TAIL:
        return zones[index.row()].long_tail;
    }
  }
  else if (role == MAX_ROLE) {
    switch (index.column()) {
      case jm::ZONE_START:
      case jm::ZONE_LEFT:
      case jm::ZONE_RIGHT:
        return (float) zones[index.row()].wave_length / sample_rate;
    }
  }
  else if (role == Qt::TextAlignmentRole) {
    switch (index.column()) {
      case jm::ZONE_AMP:
      case jm::ZONE_LOW_VEL:
      case jm::ZONE_HIGH_VEL:
      case jm::ZONE_PITCH:
      case jm::ZONE_START:
      case jm::ZONE_LEFT:
      case jm::ZONE_RIGHT:
      case jm::ZONE_CROSSFADE:
      case jm::ZONE_ATTACK:
      case jm::ZONE_HOLD:
      case jm::ZONE_DECAY:
      case jm::ZONE_SUSTAIN:
      case jm::ZONE_RELEASE: {
        // not sure why QVariant doesn't like combined aligns until casted
        return (int) (Qt::AlignRight | Qt::AlignVCenter);
        break;
      }
    }
  }
  else if (role == Qt::ToolTipRole && index.column() == jm::ZONE_PATH) {
    return zones[index.row()].path;
  }
  else if (role == Qt::DisplayRole && index.column() == jm::ZONE_PATH) {
    char tmp[MAX_PATH];
    strcpy(tmp, zones[index.row()].path);
    return basename(tmp);
  }
  else if (role == Qt::DisplayRole || role == Qt::EditRole) {
    switch (index.column()) {
      case jm::ZONE_NAME:
        return zones[index.row()].name;
      case jm::ZONE_AMP:
        return 20.f * log10f(zones[index.row()].amp);
      case jm::ZONE_ORIGIN:
        return note_to_string(zones[index.row()].origin);
      case jm::ZONE_LOW_KEY:
        return note_to_string(zones[index.row()].low_key);
      case jm::ZONE_HIGH_KEY:
        return note_to_string(zones[index.row()].high_key);
      case jm::ZONE_LOW_VEL:
        return zones[index.row()].low_vel;
      case jm::ZONE_HIGH_VEL:
        return zones[index.row()].high_vel;
      case jm::ZONE_PITCH:
        return zones[index.row()].pitch_corr;
      case jm::ZONE_START:
        return (float) zones[index.row()].start / sample_rate;
      case jm::ZONE_LEFT:
        return (float) zones[index.row()].left / sample_rate;
      case jm::ZONE_RIGHT:
        return (float) zones[index.row()].right / sample_rate;
      case jm::ZONE_LOOP_MODE:
        switch (zones[index.row()].loop_mode) {
          case jm::LOOP_OFF:
            return "off";
          case jm::LOOP_CONTINUOUS:
            return "on";
          case jm::LOOP_ONE_SHOT:
            return "one shot";
          default:
            break;
        }
        break;
      case jm::ZONE_CROSSFADE:
        // 0.5 epsilon to prevent truncation error
        return (int) ((double) zones[index.row()].crossfade / sample_rate * 1000. + 0.5);
      case jm::ZONE_GROUP:
        return zones[index.row()].group == 0 ? "none": QString::number(zones[index.row()].group);
      case jm::ZONE_OFF_GROUP:
        return zones[index.row()].off_group == 0 ? "none": QString::number(zones[index.row()].off_group);
      case jm::ZONE_ATTACK:
        return (float) zones[index.row()].attack / sample_rate;
      case jm::ZONE_HOLD:
        return (float) zones[index.row()].hold / sample_rate;
      case jm::ZONE_DECAY:
        return (float) zones[index.row()].decay / sample_rate;
      case jm::ZONE_SUSTAIN:
        return zones[index.row()].sustain;
      case jm::ZONE_RELEASE:
        return (float) zones[index.row()].release / sample_rate;
      case jm::ZONE_PATH:
        return zones[index.row()].path;
    }
  }

  return QVariant();
}
    
bool ZoneTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (index.isValid() && role == Qt::CheckStateRole) {
    switch (index.column()) {
      case jm::ZONE_MUTE:
        zones[index.row()].mute = value.toInt();
        std::cout << "update_zone:" << index.row() << "," << index.column() << "," << zones[index.row()].mute << std::endl;
        break;
      case jm::ZONE_SOLO:
        zones[index.row()].solo = value.toInt();
        std::cout << "update_zone:" << index.row() << "," << index.column() << "," << zones[index.row()].solo << std::endl;
        break;
      // long tail special case; used for UI only, don't send a message out
      case jm::ZONE_LONG_TAIL:
        zones[index.row()].long_tail = value.toInt();
        break;
    }
    emit dataChanged(index, index);
    return true;
  }
  else if (index.isValid() && role == Qt::EditRole) {
    std::cout << "update_zone:" << index.row() << "," << index.column() << ",";
    switch (index.column()) {
      case jm::ZONE_NAME:
        strcpy(zones[index.row()].name, value.toString().toStdString().c_str());
        std::cout << value.toString().toStdString();
        break;
      case jm::ZONE_AMP:
        zones[index.row()].amp = powf(10.f, value.toFloat() / 20.f);
        std::cout << zones[index.row()].amp;
        break;
      case jm::ZONE_ORIGIN:
        zones[index.row()].origin = string_to_note(value.toString());
        std::cout << zones[index.row()].origin;
        break;
      case jm::ZONE_LOW_KEY:
        zones[index.row()].low_key = string_to_note(value.toString());
        std::cout << zones[index.row()].low_key;
        break;
      case jm::ZONE_HIGH_KEY:
        zones[index.row()].high_key = string_to_note(value.toString());
        std::cout << zones[index.row()].high_key;
        break;
      case jm::ZONE_LOW_VEL:
        zones[index.row()].low_vel = value.toInt();
        std::cout << value.toInt();
        break;
      case jm::ZONE_HIGH_VEL:
        zones[index.row()].high_vel = value.toInt();
        std::cout << value.toInt();
        break;
      case jm::ZONE_PITCH:
        zones[index.row()].pitch_corr = value.toDouble();
        std::cout << value.toDouble();
        break;
      case jm::ZONE_START:
        zones[index.row()].start = (int) (value.toFloat() * sample_rate);
        std::cout << zones[index.row()].start;
        break;
      case jm::ZONE_LEFT:
        zones[index.row()].left = (int) (value.toFloat() * sample_rate);
        std::cout << zones[index.row()].left;
        break;
      case jm::ZONE_RIGHT:
        zones[index.row()].right = (int) (value.toFloat() * sample_rate);
        std::cout << zones[index.row()].right;
        break;
      case jm::ZONE_LOOP_MODE:
        if (value.toString() == "off")
          zones[index.row()].loop_mode = jm::LOOP_OFF;
        else if (value.toString() == "on")
          zones[index.row()].loop_mode = jm::LOOP_CONTINUOUS;
        else if (value.toString() == "one shot")
          zones[index.row()].loop_mode = jm::LOOP_ONE_SHOT;

        std::cout << zones[index.row()].loop_mode;
        break;
      case jm::ZONE_CROSSFADE:
        zones[index.row()].crossfade = (int) (value.toInt() * sample_rate / 1000.);
        std::cout << zones[index.row()].crossfade;
        break;
      case jm::ZONE_GROUP:
        if (value.toString() == "none")
          zones[index.row()].group = 0;
        else
          zones[index.row()].group = value.toString().toInt();
        
        std::cout << zones[index.row()].group;
        break;
      case jm::ZONE_OFF_GROUP:
        if (value.toString() == "none")
          zones[index.row()].off_group = 0;
        else
          zones[index.row()].off_group = value.toString().toInt();
        
        std::cout << zones[index.row()].off_group;
        break;
      case jm::ZONE_ATTACK:
        zones[index.row()].attack = value.toFloat() * sample_rate;
        std::cout << zones[index.row()].attack;
        break;
      case jm::ZONE_HOLD:
        zones[index.row()].hold = value.toFloat() * sample_rate;
        std::cout << zones[index.row()].hold;
        break;
      case jm::ZONE_DECAY:
        zones[index.row()].decay = value.toFloat() * sample_rate;
        std::cout << zones[index.row()].decay;
        break;
      case jm::ZONE_SUSTAIN:
        zones[index.row()].sustain = value.toFloat();
        std::cout << zones[index.row()].sustain;
        break;
      case jm::ZONE_RELEASE:
        zones[index.row()].release = value.toFloat() * sample_rate;
        std::cout << zones[index.row()].release;
        break;
      case jm::ZONE_PATH:
        strcpy(zones[index.row()].path, value.toString().toStdString().c_str());
        std::cout << value.toString().toStdString();
        break;
    }
    std::cout << std::endl;

    emit dataChanged(index, index);
    return true;
  }
  return false;
}

void ZoneTableModel::addNewZone(int i, const jm::zone& z) {
  insertRows(i, 1);
  zones[i] = z;
  emit dataChanged(index(i, 0), index(i, NUM_ZONE_ATTRS - 1));
}

void ZoneTableModel::updateWave(int i, const QString& path, int wave_length, int start, int left, int right) {
  strcpy(zones[i].path, path.toStdString().c_str());
  zones[i].wave_length = wave_length;
  zones[i].start = start;
  zones[i].left = left;
  zones[i].right = right;
}

void ZoneTableModel::removeZone(int i) {
  removeRows(i, 1);
}

void ZoneTableModel::clearZones() {
  removeRows(0, rowCount());
}

