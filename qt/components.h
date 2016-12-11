#ifndef JM_UI_COMPONENTS
#define JM_UI_COMPONENTS

#include <QWidget>

class QLineEdit;

class DragBox: public QWidget {
  Q_OBJECT

  private:
    //int val;
    QLineEdit* out;
    int prev_y;
    double scale;

  signals:
    void released(const QString& text);
    void textChanged(const QString& text);

  /*protected:
    void mousePressEvent(QMouseEvent* e);
    void mouseDoubleClickEvent(QMouseEvent* e);
    void mouseReleaseEvent(QMouseEvent* e);
    void mouseMoveEvent(QMouseEvent* e);
  */
  public:
    DragBox(double min = 0.0, double max = 100.0, int steps = 101, QWidget* parent = Q_NULLPTR);
    bool eventFilter(QObject *obj, QEvent *event);
    QString text() const;
    void setText(const QString& text);
    void setGeometry(const QRect& rect);
};

#endif
