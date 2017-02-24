TEMPLATE = lib

CONFIG += plugin
CONFIG += no_plugin_name_prefix

INCLUDEPATH += ../
LIBS += -L../lib -llib

SOURCES = jm-sampler-lv2.cpp
