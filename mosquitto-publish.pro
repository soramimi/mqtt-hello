TARGET = mosquitto-publish
TEMPLATE = app

DESTDIR = $$PWD/_bin

LIBS += -lmosquitto

SOURCES += \
	mosquitto/publish.c
