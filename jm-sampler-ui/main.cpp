#include <QApplication>

#include "jm-sampler-ui.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);
  SamplerUI sampler;
  sampler.show();
  return app.exec();
}
