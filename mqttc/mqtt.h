/* 
 * mqtt.h - mqtt client api
 *
 * Copyright (c) 2013  Ery Lee <ery.lee at gmail dot com>
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
#ifndef __MQTT_H
#define __MQTT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory>
#include <vector>

#define MQTT_OK 0
#define MQTT_ERR -1

#define MQTT_PROTO_MAJOR 3
#define MQTT_PROTO_MINOR 1

#define MQTT_PROTOCOL_VERSION "MQTT/3.1"

#define MQTT_ERR_SOCKET (-5)

/*
 * MQTT QOS
 */
#define MQTT_QOS0 0
#define MQTT_QOS1 1
#define MQTT_QOS2 2

/*
 * MQTT ConnAck
 */
enum ConnAck {
	CONNACK_ACCEPT  = 0,
	CONNACK_PROTO_VER, 
	CONNACK_INVALID_ID,
	CONNACK_SERVER,
	CONNACK_CREDENTIALS,
	CONNACK_AUTH
};

/*
 * MQTT State
 */
enum MqttState {
	MQTT_STATE_INIT = 0,
	MQTT_STATE_CONNECTING,
	MQTT_STATE_CONNECTED,
	MQTT_STATE_DISCONNECTED
};

/*
 * MQTT Will
 */
struct MqttWill {
	bool retain;
	uint8_t qos;
	std::string topic;
	std::string msg;
};

/*
 * MQTT Message
 */
struct MqttMsg {
	uint16_t id = 0;
	uint8_t qos = 0;
	bool retain = false;
	bool dup = false;
	std::string topic;
	std::vector<char> payload;
};

class Mqtt {
public:
	Mqtt()
	{
	}

	~Mqtt()
	{
		close();
	}

	void close();

	typedef void (*MqttCallback)(Mqtt *mqtt, void *data, int id);
	typedef void (*MqttMsgCallback)(Mqtt *mqtt, MqttMsg *message);

	int fd = -1; //socket
	uint8_t state = 0;
	int error = 0;
	char errstr[1024];
	std::string server;
	std::string username;
	std::string password;
	std::string clientid;
	int port = 0;
	int retries = 0;
	int msgid = 0;
	bool cleansess = false;

    /* keep alive */
	unsigned int keepalive = 0;
	long long keepalive_timer = 0;
	long long keepalive_timeout_timer = 0;

	std::shared_ptr<MqttWill> will;
	MqttCallback callbacks[16];
	MqttMsgCallback msgcallback = nullptr;
	bool shutdown_asap = false;
	int connack = 0;

	void *userdata = nullptr;

	void mqtt_read(int fd, int mask);

	void mqtt_set_clientid(const std::string &clientid);
	void mqtt_set_username(const std::string &username);
	void mqtt_set_passwd(const std::string &passwd);
	void mqtt_set_server(const std::string &server);
	void mqtt_set_port(int port);
	void mqtt_set_retries(int retries);
	void mqtt_set_cleansess(bool cleansess);
	void mqtt_set_will(const std::shared_ptr<MqttWill> &will);
	void mqtt_clear_will();
	void mqtt_set_keepalive(int keepalive);
	void mqtt_set_callback(uint8_t type, MqttCallback callback);
	void mqtt_clear_callback(uint8_t type);
	void mqtt_set_msg_callback(MqttMsgCallback callback);
	void mqtt_clear_msg_callback();
	int mqtt_connect();
	int mqtt_publish(MqttMsg *msg);
	void mqtt_puback(int msgid);
	void mqtt_pubrec(int msgid);
	void mqtt_pubrel(int msgid);
	void mqtt_pubcomp(int msgid);
	int mqtt_subscribe(const char *topic, unsigned char qos);
	int mqtt_unsubscribe(const std::string &topic);
	void mqtt_ping();
	void mqtt_disconnect();
	void mqtt_release();
	void mqtt_set_state(int state);
	static const char *mqtt_msg_name(uint8_t type);
private:
	static int _mqtt_keepalive(long long id, void *clientdata);
	void _mqtt_handle_publish(MqttMsg *msg);
	void _mqtt_handle_packet(uint8_t header, char *buffer, int buflen);
	void _mqtt_reader_feed(char *buffer, int len);
	void _mqtt_handle_puback(int type, int msgid);
	void _mqtt_handle_suback(int msgid, int qos);
	void _mqtt_handle_unsuback(int msgid);
	void _mqtt_handle_pingresp();
	void _mqtt_send_publish(MqttMsg *msg);
	void _mqtt_send_ack(int type, int msgid);
	void _mqtt_send_connect();
	void _mqtt_callback(int type, void *data, int id);
	void _mqtt_send_ping();
	void _mqtt_handle_connack(int rc);
	void _mqtt_send_unsubscribe(int msgid, const std::string &topic);
	void _mqtt_handle_publish(uint8_t header, char *buffer, int buflen);
};

std::shared_ptr<Mqtt> mqtt_new();


//Will create and release
std::shared_ptr<MqttWill> mqtt_will_new(const std::string &topic, const std::string &msg, bool retain, uint8_t qos);

//Message create and release
void mqtt_msg_new(MqttMsg *msg, int msgid, int qos, bool retain, bool dup, const std::string &topic, char const *ptr, size_t len);



#endif /* __MQTT_H__ */

