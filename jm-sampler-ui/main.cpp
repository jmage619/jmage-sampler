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
