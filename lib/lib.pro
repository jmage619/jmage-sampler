TEMPLATE = lib

QT += widgets
CONFIG += staticlib

HEADERS = wave.h zone.h collections.h components.h jmsampler.h sfzparser.h \
  lv2_uris.h gui_components.h gui_zonegrid.h
SOURCES = wave.cpp components.cpp jmsampler.cpp sfzparser.cpp gui_components.cpp \
  gui_zonegrid.cpp
