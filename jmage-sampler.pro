TEMPLATE = subdirs

SUBDIRS = lib jm-sampler-lv2 jm-sampler-ui jm-sampler-lv2exui

jm-sampler-lv2.depends = lib
jm-sampler-ui.depends = lib
jm-sampler-lv2exui.depends = lib
