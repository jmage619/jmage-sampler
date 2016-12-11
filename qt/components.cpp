#include <iostream>
#include <cstdio>
#include <QtWidgets>

#include "components.h"

DragBox::DragBox(double min, double max, int steps, QWidget* parent):
    QWidget(parent),
    scale((max - min) / (steps - 1)) {
  out = new QLineEdit(this);
  out->setMouseTracking(false);
  out->setFocusPolicy(Qt::NoFocus); // super important, qlineedit focus change confuses qtableview
  out->setText("0");
  out->installEventFilter(this);
  connect(out, &QLineEdit::textChanged, this, &DragBox::textChanged);
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
      emit released(text_box->text());
      return true;
    case QEvent::MouseMove: {
      mouse_event = static_cast<QMouseEvent*>(event);
      double val = text_box->text().toDouble();
      // update displayed value on mouse move
      //printf("dbox moved; old pos: %i\n", prev_y);
      text_box->setText(QString::number(val + scale * (prev_y - mouse_event->pos().y())));
      prev_y = mouse_event->pos().y();
      //printf("dbox moved; new pos: %i\n", prev_y);
      return true;
    }
    default:
      return QObject::eventFilter(obj, event);
  }
}
QString DragBox::text() const {
  return out->text();
}

void DragBox::setText(const QString& text) {
  out->setText(text);  
}

void DragBox::setGeometry(const QRect& rect) {
  QWidget::setGeometry(rect);
  out->resize(rect.size());
}
