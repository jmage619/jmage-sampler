#ifndef JM_UI_COMPONENTS
#define JM_UI_COMPONENTS

#include <QWidget>

class QLineEdit;

class DragBox: public QWidget {
  Q_OBJECT

  private:
    //int val;
    int index;
    int prev_y;
    double min;
    double max;
    int steps;
    QLineEdit* out;

  signals:
    void released(double value);
    void dragged(double value);

  /*protected:
    void mousePressEvent(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
  */
  public:
    DragBox(QWidget* parent = Q_NULLPTR, double min = 0.0, double max = 100.0, int steps = 101);
    bool eventFilter(QObject *obj, QEvent *event);
    void setGeometry(const QRect& rect);
    void setValue(double val);
    double value();
};

#endif
