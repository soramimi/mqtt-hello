TEMPLATE = app
TARGET = paho-subscribe
CONFIG += console

DESTDIR = $$PWD/_bin

HEADERS += \
	paho/MQTTConnect.h \
	paho/MQTTFormat.h \
	paho/MQTTPacket.h \
	paho/MQTTPublish.h \
	paho/MQTTSubscribe.h \
	paho/MQTTUnsubscribe.h \
	paho/StackTrace.h \
	paho/MqttClient.h
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
	paho/subscribe.cpp \
