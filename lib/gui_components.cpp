#include <iostream>
#include <cstdio>
#include <QtWidgets>

#include "gui_components.h"

/************
*
* HDoubleSlider
*
**/

void HDoubleSlider::handleMove(int i) {
  double val = indexToVal(i);
  out->setText(QString::number(val));
  emit sliderMoved(val);
}

HDoubleSlider::HDoubleSlider(QWidget* parent, double min, double max, int steps):
    QWidget(parent),
    min(min),
    max(max) {
  QHBoxLayout* h_layout = new QHBoxLayout;
  out = new QLineEdit;
  out->setFixedWidth(50);
  out->setAlignment(Qt::AlignRight);
  h_layout->addWidget(out, 0, Qt::AlignLeft);

  slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(0);
  slider->setMaximum(steps - 1);
  slider->setFixedWidth(200);
  h_layout->addWidget(slider, 0, Qt::AlignLeft);

  out->setText(QString::number(indexToVal(0)));

  setLayout(h_layout);

  connect(slider, &QSlider::sliderMoved, this, &HDoubleSlider::handleMove);
}

void HDoubleSlider::setValue(double val) {
  if (val < min)
    slider->setValue(0);
  else if (val > max)
    slider->setValue(slider->maximum());
  // 0.05 is epsilon to deal with truncation errors...might be too big?
  else
    slider->setValue(slider->maximum() * (val - min) / (max - min) + 0.05);

  out->setText(QString::number(value()));
}

/************
*
* DragBox
*
**/

void DragBox::mousePressEvent(QMouseEvent*) {
  showPopup();
}

DragBox::DragBox(QWidget* parent, double min, double max, int steps):
    QFrame(parent),
    index(0),
    min(min),
    max(max),
    steps(steps) {
  setFrameStyle(QFrame::Box | QFrame::Plain);
  setLineWidth(2);
  out = new QLineEdit(this);
  out->setWindowFlags(Qt::Popup);
  out->setText("0");
  out->installEventFilter(this);
}

bool DragBox::eventFilter(QObject* obj, QEvent* event) {
  QLineEdit* text_box = static_cast<QLineEdit*>(obj);
  QMouseEvent* mouse_event;
  switch (event->type()) {
    case QEvent::MouseButtonRelease:
      //printf("dbox released\n");
      out->hide();
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

void DragBox::showPopup() {
  prev_y = mapFromGlobal(QCursor::pos()).y();
  out->move(mapToGlobal(QPoint(0,0)));
  out->show();
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

/************
*
* NotePopup
*
**/

void NotePopup::setIndexFromAction(QAction* action) {
  text = action->text();

  emit selected(text);
}

NotePopup::NotePopup(QWidget* parent): QFrame(parent) {
  this->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->setLineWidth(2);

  menu = new QMenu(this);

  for (int i = -1; i <= 9; ++i) {
    QMenu* submenu = menu->addMenu(QString::number(i));

    submenu->addAction(QString::number(i) + "C");
    submenu->addAction(QString::number(i) + "C#");
    submenu->addAction(QString::number(i) + "D");
    submenu->addAction(QString::number(i) + "D#");
    submenu->addAction(QString::number(i) + "E");
    submenu->addAction(QString::number(i) + "F");
    submenu->addAction(QString::number(i) + "F#");
    submenu->addAction(QString::number(i) + "G");
    if (i == 9)
      break;
    submenu->addAction(QString::number(i) + "G#");
    submenu->addAction(QString::number(i) + "A");
    submenu->addAction(QString::number(i) + "A#");
    submenu->addAction(QString::number(i) + "B");
  }
  connect(menu, &QMenu::triggered, this, &NotePopup::setIndexFromAction);
}

void NotePopup::showPopup() {
  menu->setFixedWidth(width());

  // line up height below cell
  // eventually figure out how to draw box in cur cell for visibility
  menu->popup(mapToGlobal(QPoint(0, height())));
  //menu->popup(QCursor::pos());
}

QString NotePopup::currentText() {
  return text;
}

void NotePopup::setCurrentText(const QString& text) {
  this->text = text;

  if (menu->isVisible()) {
    // first octave is -1, add 1 to make it a valid index
    int octave_index = text.startsWith("-1") ? 0: text[0].digitValue() + 1;
    QAction* sub_action = menu->actions().at(octave_index);
    menu->setActiveAction(sub_action);
    QMenu* submenu  = sub_action->menu();

    QList<QAction*> sub_actions = submenu->actions();

    for (int i = 0; i < sub_actions.size(); ++i) {
      if (sub_actions.at(i)->text() == text) {
        submenu->setActiveAction(sub_actions.at(i));
        break;
      }
    }
  }
}

void NotePopup::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    showPopup();
  }
}
