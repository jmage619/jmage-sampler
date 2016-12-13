#include <iostream>
#include <cstdio>
#include <QtWidgets>

#include "components.h"

DragBox::DragBox(QWidget* parent, double min, double max, int steps):
    QWidget(parent),
    index(0),
    min(min),
    max(max),
    steps(steps) {
  out = new QLineEdit(this);
  out->setMouseTracking(false);
  out->setFocusPolicy(Qt::NoFocus); // super important, qlineedit focus change confuses qtableview
  out->setText("0");
  out->installEventFilter(this);
}

bool DragBox::eventFilter(QObject* obj, QEvent* event) {
  QLineEdit* text_box = static_cast<QLineEdit*>(obj);
  QMouseEvent* mouse_event;
  switch (event->type()) {
    case QEvent::MouseButtonPress: 
    case QEvent::MouseButtonDblClick: // needed to play nice with QTableView mechanics
      mouse_event = static_cast<QMouseEvent*>(event);
      // capture y on click
      prev_y = mouse_event->pos().y();
      //printf("dbox clicked; pos: %i\n", prev_y);
      return true;
    case QEvent::MouseButtonRelease:
      //printf("dbox released\n");
      emit released(value());
      return true;
    case QEvent::MouseMove: {
      mouse_event = static_cast<QMouseEvent*>(event);
      // update value on mouse move
      int prev_index = index;
      index += prev_y - mouse_event->pos().y();

      if (index < 0)
        index = 0;
      else if (index >= steps)
        index = steps - 1;

      prev_y = mouse_event->pos().y();

      if (index != prev_index) {
        text_box->setText(QString::number(value()));
        emit dragged(value());
      }
      return true;
    }
    default:
      return QObject::eventFilter(obj, event);
  }
}

void DragBox::setGeometry(const QRect& rect) {
  QWidget::setGeometry(rect);
  out->resize(rect.size());
}

void DragBox::setValue(double val) {
  if (val < min)
    index = 0;
  else if (val > max)
    index = steps - 1;
  // 0.05 is epsilon to deal with truncation errors...might be too big?
  else
    index = (steps - 1) * (val - min) / (max - min) + 0.05;

  out->setText(QString::number(value()));
}

double DragBox::value() {
  return (max - min) / (steps - 1) * index + min;
}
