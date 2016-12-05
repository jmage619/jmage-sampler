#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>
#include <QThread>
#include <QAbstractTableModel>
#include <QString>

#define NUM_ZONE_ATTRS 12

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
  JM_ZONE_LOOP_MODE
};

enum loop_mode {
  LOOP_OFF,
  LOOP_CONTINUOUS,
  LOOP_ONE_SHOT
};

// don't forget whenever we add an item to increase size in NUM_ZONE_ATTRS 
struct zone {
  QString name;
  QString amp;
  QString origin;
  QString low_key;
  QString high_key;
  QString low_vel;
  QString high_vel;
  QString pitch;
  QString start;
  QString left;
  QString right;
  QString loop_mode;
};

Q_DECLARE_METATYPE(zone)

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

  public:
    SamplerUI();

  public slots:
    void showAndRaise();
    void addNewZone(const zone& z);
};

#endif
