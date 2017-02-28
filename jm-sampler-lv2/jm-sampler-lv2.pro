TEMPLATE = lib

CONFIG -= qt
CONFIG += plugin no_plugin_name_prefix

INCLUDEPATH += ../
LIBS += -L../lib -llib -lsndfile -lsamplerate

HEADERS = plugin.h
SOURCES = jm-sampler-lv2.cpp plugin.cpp
