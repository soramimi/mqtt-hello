TARGET = mosquitto-subscribe
TEMPLATE = app

DESTDIR = $$PWD/_bin

LIBS += -lmosquitto

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
	mosquitto/subscribe.c
