#ifndef JM_UI_ZONEGRID
#define JM_UI_ZONEGRID

#include <QThread>
#include <QTableView>
#include <QStyledItemDelegate>
#include <QAbstractTableModel>

class QString;

#define NUM_ZONE_ATTRS 22

enum zone_params {
  JM_ZONE_NAME,
  JM_ZONE_AMP,
  JM_ZONE_ORIGIN,
  JM_ZONE_LOW_KEY,
  JM_ZONE_HIGH_KEY,
  JM_ZONE_LOW_VEL,
  JM_ZONE_HIGH_VEL,
  JM_ZONE_PITCH,
  JM_ZONE_START,
  JM_ZONE_LEFT,
  JM_ZONE_RIGHT,
  JM_ZONE_LOOP_MODE,
  JM_ZONE_CROSSFADE,
  JM_ZONE_GROUP,
  JM_ZONE_OFF_GROUP,
  JM_ZONE_ATTACK,
  JM_ZONE_HOLD,
  JM_ZONE_DECAY,
  JM_ZONE_SUSTAIN,
  JM_ZONE_RELEASE,
  JM_ZONE_LONG_TAIL,
  JM_ZONE_PATH
};

enum roles {
  JM_MAX_ROLE = Qt::UserRole
};

enum loop_mode {
  LOOP_OFF,
  LOOP_CONTINUOUS,
  LOOP_ONE_SHOT
};

// don't forget whenever we add an item to increase size in NUM_ZONE_ATTRS 
struct zone {
  double wave_length; // don't count as ZONE_ATTR, not used in ui
  QString name;
  double amp;
  QString origin;
  QString low_key;
  QString high_key;
  double low_vel;
  double high_vel;
  double pitch;
  double start;
  double left;
  double right;
  QString loop_mode;
  double crossfade;
  QString group;
  QString off_group;
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
