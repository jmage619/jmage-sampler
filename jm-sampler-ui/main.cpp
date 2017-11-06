#include <cstdio>
#include <string>
#include <stdexcept>
#include <QApplication>

#include "jm-sampler-ui.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  SamplerUI sampler;
  if (argc > 1)
    sampler.setWindowTitle(argv[1]);
  else
    sampler.setWindowTitle("JMAGE Sampler");

  sampler.show();
  return app.exec();
}
