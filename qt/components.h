#ifndef JM_UI_COMPONENTS
#define JM_UI_COMPONENTS

#include <QLineEdit>

class QMouseEvent;

class DragBox: public QLineEdit {
  Q_OBJECT

  private:
    //int val;
    int prev_y;
    double scale;

  signals:
    void released(const QString& text);

  protected:
    void mousePressEvent(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
  public:
    DragBox(double min = 0.0, double max = 100.0, int steps = 101, QWidget* parent = Q_NULLPTR);
};

#endif
