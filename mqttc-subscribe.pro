
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
	mqttc/anet.cpp \
	mqttc/client.cpp \
	mqttc/mqtt.cpp \
	mqttc/packet.cpp \
	mqttc/subscribe.cpp
