TEMPLATE = lib

CONFIG += plugin
CONFIG += no_plugin_name_prefix

INCLUDEPATH += ../
LIBS += -L../lib -llib

HEADERS = lv2_external_ui.h
SOURCES = jm-sampler-lv2exui.c
