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

#ifndef GUI_ZONEGRID
#define GUI_ZONEGRID

#include <QThread>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QAbstractTableModel>

#include <lib/zone.h>

class QString;

#define NUM_ZONE_ATTRS 24

enum roles {
  MAX_ROLE = Qt::UserRole
};

// some helper functions
QString note_to_string(int note);
int string_to_note(const QString& str);

class MouseHandleView: public QTableView {
  Q_OBJECT

  protected:
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);

  signals:
    void userUpdate();

  public:
    MouseHandleView(QWidget* parent = Q_NULLPTR): QTableView(parent) {}
};

class ZoneTableView: public MouseHandleView {
  Q_OBJECT

  private:
    MouseHandleView* frozen_view;
    void updateFrozenTableGeometry();
    void init();
  protected:
    virtual void resizeEvent(QResizeEvent* event);
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible);
  public:
    ZoneTableView(QAbstractItemModel* model);
    bool eventFilter(QObject *obj, QEvent *event);
    void setSelectionMode(QAbstractItemView::SelectionMode mode);
  private slots:
    void updateSectionWidth(int logicalIndex, int oldSize, int newSize);
    void updateFrozenSectionWidth(int logicalIndex, int oldSize, int newSize);
    void updateSectionHeight(int logicalIndex, int oldSize, int newSize);
    void handleVertHeaderClick(const QPoint& pos);
};

class ZoneTableDelegate: public QStyledItemDelegate {
  Q_OBJECT

  private:
    int precision;

  public:
    ZoneTableDelegate(QObject* parent = Q_NULLPTR, int precision = 2):
      QStyledItemDelegate(parent), precision(precision) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
        const QModelIndex& index) const;
    void updateEditorGeometry(QWidget* editor,
        const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
        const QModelIndex& index) const;
    QString displayText(const QVariant &value, const QLocale &locale) const;

  public slots:
    void updateData();
    void forceClose();
    void commitAndCloseEditor();

};

class ZoneTableModel: public QAbstractTableModel {
  Q_OBJECT

  private:
    int sample_rate;
    std::vector<jm::zone> zones;
  public:
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  public slots:
    void setSampleRate(int sample_rate) {this->sample_rate = sample_rate;}
    void addNewZone(int i, const jm::zone& z);
    void updateWave(int i, const QString& path, int wave_length, int start, int left, int right);
    void removeZone(int i);
    void clearZones();
};

#endif
