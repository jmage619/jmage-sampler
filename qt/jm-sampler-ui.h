#ifndef JM_SAMPLER_UI
#define JM_SAMPLER_UI

#include <QWidget>
#include <QThread>
#include <QAbstractTableModel>
#include <QString>

#define NUM_ZONE_ATTRS 3

enum zone_params {
  JM_ZONE_NAME,
  JM_ZONE_AMP,
  JM_ZONE_ORIGIN
};

// don't forget whenever we add an item to increase size in NUM_ZONE_ATTRS 
struct zone {
  QString name;
  QString amp;
  QString origin;
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
