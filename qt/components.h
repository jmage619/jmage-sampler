#ifndef JM_UI_COMPONENTS
#define JM_UI_COMPONENTS

#include <QWidget>
#include <QFrame>

class QLineEdit;
class QAction;
class QMenu;

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
#endif
