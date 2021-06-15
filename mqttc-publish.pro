
TEMPLATE = app
TARGET = mqttc-publish
CONFIG += console
DESTDIR = $$PWD/_bin


HEADERS += \
	mqttc/anet.h \
	mqttc/client.h \
	mqttc/config.h \
	mqttc/mqtt.h \
	mqttc/packet.h \
	mqttserver.h

SOURCES += \
	mqttc/anet.c \
	mqttc/client.c \
	mqttc/publish.c \
	mqttc/mqtt.c \
	mqttc/packet.c
