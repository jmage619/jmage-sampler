#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>
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
  QString start;
  QString left;
  QString right;
  QString loop_mode;
  QString crossfade;
  QString group;
  QString off_group;
  QString attack;
  QString hold;
  QString decay;
  QString sustain;
  QString release;
  int long_tail;
  QString path;
};

Q_DECLARE_METATYPE(zone)

class ZoneTableView: public QTableView {
  Q_OBJECT

  protected:
    void mousePressEvent(QMouseEvent *event);
};

class ZoneTableDelegate: public QStyledItemDelegate {
  Q_OBJECT

  public:
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

class InputThread: public QThread {
  Q_OBJECT

  public:
    void run();
  signals:
    //void receivedValue(int val);
    void receivedShow();
    void receivedHide();
    void receivedAddZone(const zone& z);
};

class SamplerUI: public QWidget {
  Q_OBJECT

  private:
    ZoneTableModel zone_model;
    ZoneTableDelegate delegate;

  public:
    SamplerUI();

  public slots:
    void showAndRaise();
    void addNewZone(const zone& z);
};

#endif
