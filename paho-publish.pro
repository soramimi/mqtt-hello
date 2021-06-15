TEMPLATE = app
TARGET = paho-publish
CONFIG += console

DESTDIR = $$PWD/_bin

HEADERS += \
	paho/MQTTConnect.h \
	paho/MQTTFormat.h \
	paho/MQTTPacket.h \
	paho/MQTTPublish.h \
	paho/MQTTSubscribe.h \
	paho/MQTTUnsubscribe.h \
	paho/MqttClient.h \
	paho/StackTrace.h
SOURCES += \
	paho/MQTTConnectClient.c \
	paho/MQTTConnectServer.c \
	paho/MQTTDeserializePublish.c \
	paho/MQTTFormat.c \
	paho/MQTTPacket.c \
	paho/MQTTSerializePublish.c \
	paho/MQTTSubscribeClient.c \
	paho/MQTTSubscribeServer.c \
	paho/MQTTUnsubscribeClient.c \
	paho/MQTTUnsubscribeServer.c \
	paho/MqttClient.cpp \
	paho/publish.cpp
