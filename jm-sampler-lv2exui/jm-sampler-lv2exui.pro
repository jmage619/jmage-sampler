TEMPLATE = lib

CONFIG -= qt
CONFIG += plugin no_plugin_name_prefix

INCLUDEPATH += ../
LIBS += -L../lib -llib -lm

HEADERS = lv2_external_ui.h
SOURCES = jm-sampler-lv2exui.c
