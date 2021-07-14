TARGET = mosquitto-tls-subscribe
TEMPLATE = app

DESTDIR = $$PWD/_bin

LIBS += -lmosquitto

SOURCES += \
    mosquitto-tls/subscribe.c
