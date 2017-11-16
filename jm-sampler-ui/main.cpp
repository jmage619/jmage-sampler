// main.cpp
//
/****************************************************************************
    Copyright (C) 2017  jmage619

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*****************************************************************************/

#include <cstdio>
#include <string>
#include <stdexcept>
#include <QApplication>

#include "jm-sampler-ui.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  SamplerUI sampler;
  if (argc > 1)
    sampler.setWindowTitle(QString(argv[1]) + "[*]");
  else
    sampler.setWindowTitle(QString("JMAGE Sampler") + "[*]");

  sampler.resize(660, 430);

  sampler.show();
  return app.exec();
}
