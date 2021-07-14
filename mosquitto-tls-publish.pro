TARGET = mosquitto-tls-publish
TEMPLATE = app

DESTDIR = $$PWD/_bin

LIBS += -lmosquitto

SOURCES += \
    mosquitto-tls/publish.c
