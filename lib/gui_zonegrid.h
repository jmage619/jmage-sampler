#ifndef GUI_ZONEGRID
#define GUI_ZONEGRID

#include <QThread>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QAbstractTableModel>

#include "zone.h"

class QString;

#define NUM_ZONE_ATTRS 22

enum roles {
  MAX_ROLE = Qt::UserRole
};

// don't forget whenever we add an item to increase size in NUM_ZONE_ATTRS 
struct zone {
  int wave_length; // don't count as ZONE_ATTR, not used in ui
  char name[MAX_NAME];
  float amp;
  int origin;
  int low_key;
  int high_key;
  int low_vel;
  int high_vel;
  double pitch_corr;
  int start;
  int left;
  int right;
  jm_loop_mode mode;
  int crossfade;
  int group;
  int off_group;
  double attack;
  double hold;
  double decay;
  double sustain;
  double release;
  int long_tail;
  QString path;
};

Q_DECLARE_METATYPE(zone)

// some helper functions
QString note_to_string(int note);
int string_to_note(const QString& str);

class SingleClickView: public QTableView {
  Q_OBJECT

  protected:
    void mousePressEvent(QMouseEvent *event);

  public:
    SingleClickView(QWidget* parent = Q_NULLPTR): QTableView(parent) {}
};

class ZoneTableView: public SingleClickView {
  Q_OBJECT

  private:
    SingleClickView* frozen_view;
    void updateFrozenTableGeometry();
    void init();
  protected:
    virtual void resizeEvent(QResizeEvent* event);
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers);
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible);
  public:
    ZoneTableView(QAbstractItemModel* model);
    void setSelectionMode(QAbstractItemView::SelectionMode mode);
  private slots:
    void updateSectionWidth(int logicalIndex, int oldSize, int newSize);
    void updateFrozenSectionWidth(int logicalIndex, int oldSize, int newSize);
    void updateSectionHeight(int logicalIndex, int oldSize, int newSize);
};

class ZoneTableDelegate: public QStyledItemDelegate {
  Q_OBJECT

  public:
    ZoneTableDelegate(QObject* parent = Q_NULLPTR): QStyledItemDelegate(parent) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
        const QModelIndex& index) const;
    void updateEditorGeometry(QWidget* editor,
        const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model,
        const QModelIndex& index) const;

  public slots:
    void updateData();
    void forceClose();
    void commitAndCloseEditor();
};

class ZoneTableModel: public QAbstractTableModel {
  Q_OBJECT

  private:
    std::vector<zone> zones;
  public:
    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    void setZone(int row, const zone& z);
};

#endif
