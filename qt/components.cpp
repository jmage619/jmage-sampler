#include <iostream>
#include <cstdio>
#include <QtWidgets>

#include "components.h"

void DragBox::mousePressEvent(QMouseEvent* e) {
  // capture y on click
  prev_y = e->pos().y();
  //printf("dbox clicked; pos: %i\n", prev_y);
}

// needed to play nice with QTableView mechanics
void DragBox::mouseDoubleClickEvent(QMouseEvent* e) {
  //printf("dbox double clicked!\n");
  mousePressEvent(e);
}

void DragBox::mouseReleaseEvent(QMouseEvent*) {
  //printf("dbox released\n");
  emit released(text());
}

void DragBox::mouseMoveEvent(QMouseEvent* e) {
  double val = text().toDouble();
  // update displayed value on mouse move
  //printf("dbox moved; old pos: %i\n", prev_y);
  setText(QString::number(val + scale * (prev_y - e->pos().y())));
  prev_y = e->pos().y();
  //printf("dbox moved; new pos: %i\n", prev_y);
}

DragBox::DragBox(double min, double max, int steps, QWidget* parent):
    QLineEdit(parent),
    scale((max - min) / (steps - 1)) {
  setMouseTracking(false);
  setText("0");
}

