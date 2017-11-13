#ifndef GUI_COMPONENTS_H
#define GUI_COMPONENTS_H

#include <QWidget>
#include <QFrame>
#include <QSlider>

class QLineEdit;
class QAction;
class QMenu;
class QFileDialog;

int vol_map_find(const double* map, double val);

class HVolumeSlider: public QWidget {
  Q_OBJECT

  private:
    const double* map;

    QLineEdit* out;
    QSlider* slider;
    void updateText();

  signals:
    void sliderMoved(double val);

  private slots:
    void handleMove(int val);

  public:
    int index;
    HVolumeSlider(QWidget* parent = Q_NULLPTR);
    double value(){return map[index];}
    bool eventFilter(QObject *obj, QEvent *event);

  public slots:
    void setValue(double val);
};

class Control {
  protected:
    int steps;
    int index;

  public:
    Control(int steps): steps(steps) {}
    virtual ~Control(){}
    virtual void setValue(double val) = 0;
    virtual double value() = 0;
    void increase(int i);
};

class LinearControl: public Control {
  private:
    double min;
    double max;

  public:
    LinearControl(double min = 0.0, double max = 100.0, int steps = 101);
    void setValue(double val);
    double value();
};

class VolumeControl: public Control {
  private:
    const double* map;

  public:
    VolumeControl();
    void setValue(double val) {index = vol_map_find(map, val);}
    double value(){return map[index];}
};

class DragBox: public QFrame {
  Q_OBJECT

  private:
    int prev_y;
    QLineEdit* out;
    int precision;

  protected:
    int index;
    int steps;

  signals:
    void released(double value);
    void dragged(double value);

  protected:
    void mousePressEvent(QMouseEvent* e);

  public:
    DragBox(int steps, QWidget* parent = Q_NULLPTR, int precision = 0);
    bool eventFilter(QObject *obj, QEvent *event);
    void setGeometry(const QRect& rect);
    void showPopup();
    void updateText();
    virtual void setValue(double val) = 0;
    virtual double value() = 0;
};

class LinearDragBox: public DragBox {
  Q_OBJECT

  private:
    double min;
    double max;

  public:
    LinearDragBox(QWidget* parent = Q_NULLPTR, double min = 0.0, double max = 100.0, int steps = 101, int precision = 0);
    void setValue(double val);
    double value();
};

class VolumeDragBox: public DragBox {
  Q_OBJECT

  private:
    const double* map;

  public:
    VolumeDragBox(QWidget* parent = Q_NULLPTR);
    void setValue(double val) {index = vol_map_find(map, val);updateText();}
    double value(){return map[index];}
};

class NotePopup: public QFrame {
  Q_OBJECT

  private:
    QString text;
    QMenu* menu;

  signals:
    void selected(const QString& text);

  private slots:
    void setIndexFromAction(QAction* action);

  public:
    NotePopup(QWidget* parent = Q_NULLPTR);
    void showPopup();
    QString currentText();
    void setCurrentText(const QString& text);
    void mousePressEvent(QMouseEvent *event);
};

class WavPopup: public QFrame {
  Q_OBJECT

  private:
    QFileDialog* file_dialog;

  signals:
    void accepted();

  public:
    WavPopup(QWidget* parent = Q_NULLPTR);
    void showPopup();
    void setPath(const QString& path);
    QString path();
    void mousePressEvent(QMouseEvent *event);
};

#endif
