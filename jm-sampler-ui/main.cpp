#include <cstdio>
#include <string>
#include <stdexcept>
#include <QApplication>

#include "jm-sampler-ui.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  SamplerUI sampler;
  sampler.show();
  return app.exec();
}
