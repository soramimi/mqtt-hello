
TEMPLATE = app
TARGET = mqttc-subscribe
CONFIG += console
DESTDIR = $$PWD/_bin


HEADERS += \
	mqttc/anet.h \
	mqttc/config.h \
	mqttc/mqtt.h \
	mqttc/packet.h \
	mqttc/client.h

SOURCES += \
	mqttc/anet.c \
	mqttc/subscribe.c \
	mqttc/mqtt.c \
	mqttc/packet.c \
	mqttc/client.c
