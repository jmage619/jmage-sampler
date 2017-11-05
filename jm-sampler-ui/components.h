#ifndef GUI_COMPONENTS_H
#define GUI_COMPONENTS_H

#include <QWidget>
#include <QFrame>
#include <QSlider>

class QLineEdit;
class QAction;
class QMenu;
class QFileDialog;

class HDoubleSlider: public QWidget {
  Q_OBJECT

  private:
    double min;
    double max;
    QLineEdit* out;
    QSlider* slider;

    double indexToVal(int i) {return (max - min) / slider->maximum() * i + min;}

  signals:
    void sliderMoved(double val);

  private slots:
    void handleMove(int val);

  public:
    HDoubleSlider(QWidget* parent = Q_NULLPTR, double min = 0.0, double max = 100.0, int steps = 101);
    double value(){return indexToVal(slider->value());}

  public slots:
    void setValue(double val);
};

class HVolumeSlider: public QWidget {
  Q_OBJECT

  private:
    double map[100];

    QLineEdit* out;
    QSlider* slider;

  signals:
    void sliderMoved(double val);

  private slots:
    void handleMove(int val);

  public:
    int index;
    HVolumeSlider(QWidget* parent = Q_NULLPTR);
    double value(){return map[index];}

  public slots:
    void setValue(double val);
};

class DragBox: public QFrame {
  Q_OBJECT

  private:
    int prev_y;
    QLineEdit* out;

  protected:
    int index;
    int steps;

  signals:
    void released(double value);
    void dragged(double value);

  protected:
    void mousePressEvent(QMouseEvent* e);

  public:
    DragBox(int steps, QWidget* parent = Q_NULLPTR);
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
    LinearDragBox(QWidget* parent = Q_NULLPTR, double min = 0.0, double max = 100.0, int steps = 101);
    void setValue(double val);
    double value();
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
