#include <iostream>
#include <cstdio>
#include <QtWidgets>
#include <libgen.h>
#include <cmath>

#include "components.h"
#include "mappings.h"

int vol_map_find(const double* map, double val) {
  int index;
  if (val < map[0])
    index = 0;
  else if (val >= map[99])
    index = 99;

  else {
    for (int i = 0; i <= 98; ++i) {
      if (val >= map[i] && val < map[i + 1]) {
        index = (val - map[i] < map[i + 1] - val) ? i: i + 1;
        break;
      }
    }
  }

  return index;
}

/************
*
* HVolumeSlider
*
**/

void HVolumeSlider::updateText() {
  out->setText(QString::number(value(), 'f', 1));
}

void HVolumeSlider::handleMove(int i) {
  index = i;
  updateText();
  emit sliderMoved(map[index]);
}

HVolumeSlider::HVolumeSlider(QWidget* parent):
    QWidget(parent), map(VOLUME_MAP), index(87) {

  QHBoxLayout* h_layout = new QHBoxLayout;
  out = new QLineEdit;
  out->setFixedWidth(57);
  out->setAlignment(Qt::AlignRight);
  h_layout->addWidget(out, 0, Qt::AlignLeft);

  slider = new QSlider(Qt::Horizontal);
  slider->setMinimum(0);
  slider->setMaximum(99);
  slider->setFixedWidth(200);
  slider->setValue(index);
  h_layout->addWidget(slider, 0, Qt::AlignLeft);

  slider->installEventFilter(this);

  updateText();

  setLayout(h_layout);

  connect(slider, &QSlider::sliderMoved, this, &HVolumeSlider::handleMove);
}

bool HVolumeSlider::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent* me = static_cast<QMouseEvent*>(event);
    if (me->button() == Qt::MidButton) {
      setValue(0.0);
      emit sliderMoved(map[index]);
      return true;
    }
  }
  if (event->type() == QEvent::Wheel) {
    QWheelEvent* we = static_cast<QWheelEvent*>(event);
    // mouse wheel clicks are 120 at a time
    int delta = we->angleDelta().y() / 120;

    if (index + delta > 99)
      index = 99;
    else if (index + delta < 0)
      index = 0;
    else
      index += delta;

    slider->setValue(index);
    updateText();
    emit sliderMoved(map[index]);

    we->accept();
    return true;
  }

  return QObject::eventFilter(obj, event);
}

void HVolumeSlider::setValue(double val) {
  index = vol_map_find(map, val);

  slider->setValue(index);
  updateText();
}

/************
*
* non-UI controls
*
**/

void Control::increase(int i) {
  if (index + i > steps - 1)
    index = steps - 1;
  else if (index + i < 0)
    index = 0;
  else
    index += i;
}

LinearControl::LinearControl(double min, double max, int steps):
    Control(steps),
    min(min),
    max(max) {
  setValue(min);
}

void LinearControl::setValue(double val) {
  if (val < min)
    index = 0;
  else if (val > max)
    index = steps - 1;
  // 0.05 is epsilon to deal with truncation errors...might be too big?
  else
    index = (steps - 1) * (val - min) / (max - min) + 0.05;
}

double LinearControl::value() {
  return (max - min) / (steps - 1) * index + min;
}

VolumeControl::VolumeControl():
    Control(100),
    map(VOLUME_MAP) {
  setValue(87);
}

/************
*
* Drag boxes
*
**/

void DragBox::mousePressEvent(QMouseEvent*) {
  showPopup();
}

DragBox::DragBox(int steps, QWidget* parent, int precision):
    QFrame(parent),
    precision(precision),
    steps(steps) {
  setFrameStyle(QFrame::Box | QFrame::Plain);
  setLineWidth(2);
  out = new QLineEdit(this);
  out->setAlignment(Qt::AlignRight);
  out->setWindowFlags(Qt::Popup);
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

void DragBox::updateText() {
  out->setText(QString::number(value(), 'f', precision));
}

LinearDragBox::LinearDragBox(QWidget* parent, double min, double max, int steps, int precision):
    DragBox(steps, parent, precision),
    min(min),
    max(max) {
  setValue(min);
}

void LinearDragBox::setValue(double val) {
  if (val < min)
    index = 0;
  else if (val > max)
    index = steps - 1;
  // 0.05 is epsilon to deal with truncation errors...might be too big?
  else
    index = (steps - 1) * (val - min) / (max - min) + 0.05;

  updateText();
}

double LinearDragBox::value() {
  return (max - min) / (steps - 1) * index + min;
}

VolumeDragBox::VolumeDragBox(QWidget* parent):
    DragBox(100, parent, 1), map(VOLUME_MAP) {
  setValue(87);
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

/************
*
* WavPopup
*
**/
WavPopup::WavPopup(QWidget* parent): QFrame(parent) {
  this->setFrameStyle(QFrame::Box | QFrame::Plain);
  this->setLineWidth(2);

  file_dialog = new QFileDialog(this, tr("Open a FUCKING WAV already!!"));
  file_dialog->setModal(true);
  file_dialog->setFileMode(QFileDialog::ExistingFile);
  file_dialog->setNameFilter(tr("Sound Files (*.wav *.aiff *.flac)"));

  // use accepted instead of fileSelected because fileSel sends 2x (GNOME BUG?)
  connect(file_dialog, &QDialog::accepted, this, &WavPopup::accepted);
}

void WavPopup::showPopup() {
  file_dialog->show();
}

QString WavPopup::path() {
  return file_dialog->selectedFiles()[0];
}

void WavPopup::setPath(const QString& path) {
  char* tmp_str = strdup(path.toStdString().c_str());
  file_dialog->setDirectory(dirname(tmp_str));
  free(tmp_str);
  tmp_str = strdup(path.toStdString().c_str());
  file_dialog->selectFile(basename(tmp_str));
  free(tmp_str);
}

void WavPopup::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    showPopup();
  }
}
