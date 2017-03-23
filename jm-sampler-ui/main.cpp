#include <cstdio>
#include <string>
#include <stdexcept>
#include <QApplication>

#include "jm-sampler-ui.h"

int main(int argc, char *argv[]) {
  if (argc <= 1)
    throw std::runtime_error("not enough arguments");

  int sample_rate = strtol(argv[1], NULL, 10);
  if (sample_rate <= 0)
    throw std::runtime_error(std::string("invalid sample rate: ") + argv[1]);

  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  SamplerUI sampler(sample_rate);
  sampler.show();
  return app.exec();
}
