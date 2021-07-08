/*
 *
 * client.c - mqtt client main.
 *
 * Copyright (c) 2013 Ery Lee <ery.lee at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of mqttc nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "client.h"
#include <random>

#define _NOTUSED(V) ((void)V)

void Client::init()
{
	this->mqtt = mqtt_new();
	this->shutdown_asap = false;

	std::random_device seed_gen;
	std::minstd_rand engine(seed_gen());

	char clientid[23];
	for (int i = 0; i < 22; i++) {
		clientid[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"[engine() % 62];
	}
	clientid[22] = 0;

	mqtt->mqtt_set_clientid(clientid);
}

static void on_connect(Mqtt *mqtt, void *data, int state)
{
	_NOTUSED(data);
	switch(state) {
	case MQTT_STATE_CONNECTING:
		//		printf("mqttc is connecting to %s:%d...\n", mqtt->server, mqtt->port);
		break;
	case MQTT_STATE_CONNECTED:
		//		printf("mqttc is connected.\n");
		//		print_prompt();
		break;
	case MQTT_STATE_DISCONNECTED:
		//		printf("mqttc is disconnected.\n");
		break;
		//	default:
		//		printf("mqttc is in badstate.\n");
	}
}

static void on_connack(Mqtt *mqtt, void *data, int rc)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	mqtt->connack = 1;
	//	printf("received connack: code=%d\n", rc);
}

static void on_publish(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(msgid);
	MqttMsg *msg = (MqttMsg *)data;
	//	printf("publish to %s: %s\n", msg->topic, msg->payload);
}

static void on_puback(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("received puback: msgid=%d\n", msgid);
}

static void on_pubrec(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("received pubrec: msgid=%d\n", msgid);
}

static void on_pubrel(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("received pubrel: msgid=%d\n", msgid);
}

static void on_pubcomp(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("received pubcomp: msgid=%d\n", msgid);
}

static void on_subscribe(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	char *topic = (char *)data;
	//	printf("subscribe to %s: msgid=%d\n", topic, msgid);
}

static void on_suback(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("received suback: msgid=%d\n", msgid);
}

static void on_unsubscribe(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("unsubscribe %s: msgid=%d\n", (char *)data, msgid);
}

static void on_unsuback(Mqtt *mqtt, void *data, int msgid)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	//	printf("received unsuback: msgid=%d\n", msgid);
}

static void on_pingreq(Mqtt *mqtt, void *data, int id)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	_NOTUSED(id);
	//printf("send pingreq\n");
}

static void on_pingresp(Mqtt *mqtt, void *data, int id)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	_NOTUSED(id);
	//printf("received pingresp\n");
}

static void on_disconnect(Mqtt *mqtt, void *data, int id)
{
	_NOTUSED(mqtt);
	_NOTUSED(data);
	_NOTUSED(id);
	//	printf("disconnect\n");
}

static void on_message(Mqtt *mqtt, MqttMsg *msg)
{
	_NOTUSED(mqtt);
	//	printf("received message: topic=%s, payload=%s\n", msg->topic, msg->payload);
	puts(msg->payload.data());
}

void Client::set_callbacks()
{
	int i = 0, type;
	Mqtt::MqttCallback callbacks[15] = {
		nullptr,
		on_connect,
		on_connack,
		on_publish,
		on_puback,
		on_pubrec,
		on_pubrel,
		on_pubcomp,
		on_subscribe,
		on_suback,
		on_unsubscribe,
		on_unsuback,
		on_pingreq,
		on_pingresp,
		on_disconnect
	};
	for(i = 0; i < 15; i++) {
		type = (i << 4) & 0xf0;
		this->mqtt->mqtt_set_callback(type, callbacks[i]);
	}
	this->mqtt->mqtt_set_msg_callback(on_message);
}

static int setargs(char *args, char **argv)
{
	int argc = 0;
	while (isspace(*args)) ++args;
	while (*args) {
		if (argv) argv[argc] = args;
		while (*args && !isspace(*args)) ++args;
		if (argv && *args) *args++ = '\0';
		while (isspace(*args)) ++args;
		argc++;
	}
	return argc;
}

