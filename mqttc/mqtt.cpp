/* 
 * mqtt.c - mqtt client library
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
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <assert.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "anet.h"
#include "packet.h"
#include "mqtt.h"

#define MAX_RETRIES 3

#define KEEPALIVE 100

//#define KEEPALIVE_TIMEOUT (KEEPALIVE * 2)

#define MQTT_NOTUSED(V) ((void) V)

#define MQTT_BUFFER_SIZE (1024*16)

/*
 * Why Buffer? May be used on resource limited os?
 */
std::shared_ptr<Mqtt> mqtt_new()
{
	std::shared_ptr<Mqtt> mqtt = std::make_shared<Mqtt>();
	mqtt->state = MQTT_STATE_INIT;
	mqtt->cleansess = true;
	mqtt->port = 1883;
	mqtt->retries = MAX_RETRIES;
	mqtt->error = 0;
	mqtt->msgid = 1;
	mqtt->keepalive = KEEPALIVE;
	for (int i = 0; i < 16; i++) {
		mqtt->callbacks[i] = nullptr;
	}
	return mqtt;
}

void Mqtt::mqtt_set_state(int state)
{
	this->state = state;
}

void Mqtt::mqtt_set_clientid(std::string const &clientid)
{
	this->clientid = clientid;
}

void Mqtt::mqtt_set_username(std::string const &username)
{
	this->username = username;
}

void Mqtt::mqtt_set_passwd(std::string const &passwd)
{
	this->password = passwd;
}

void Mqtt::mqtt_set_server(std::string const &server)
{
	this->server = server;
}

void Mqtt::mqtt_set_port(int port)
{
	this->port = port;
}

void Mqtt::mqtt_set_retries(int retries)
{
	this->retries = retries;
}

void Mqtt::mqtt_set_cleansess(bool cleansess)
{
	this->cleansess = cleansess;
}

void Mqtt::mqtt_set_will(std::shared_ptr<MqttWill> const &will)
{
	this->will = will;
}

void Mqtt::mqtt_clear_will()
{
	this->will.reset();
}

void Mqtt::mqtt_set_keepalive(int keepalive)
{
	this->keepalive = keepalive;
}

void Mqtt::mqtt_set_callback(uint8_t type, MqttCallback callback)
{
	if (type < 0) return;
	type = (type >> 4) & 0x0F;
	if (type > 16) return;
	this->callbacks[type] = callback;
}

void Mqtt::_mqtt_callback(int type, void *data, int id)
{
	if (type < 0) return;
	type = (type >> 4) & 0x0F;
	if (type > 16) return;
	Mqtt::MqttCallback cb = this->callbacks[type];
	if (cb) cb(this, data, id);
}

void Mqtt::mqtt_clear_callback(unsigned char type)
{
	if (type >= 16) return;
	this->callbacks[type] = nullptr;
}

void Mqtt::mqtt_set_msg_callback(MqttMsgCallback callback)
{
	this->msgcallback = callback;
}

static void _mqtt_msg_callback(Mqtt *mqtt, MqttMsg *msg)
{
	if (mqtt->msgcallback) {
		mqtt->msgcallback(mqtt, msg);
	}
}

void Mqtt::mqtt_clear_msg_callback()
{
	this->msgcallback = nullptr;
}

static void _mqtt_set_error(char *err, const char *fmt, ...)
{
	va_list ap;
	if (!err) return;
	va_start(ap, fmt);
	vsnprintf(err, 1023, fmt, ap);
	va_end(ap);
}

void Mqtt::_mqtt_send_connect()
{
	int len = 0;
	char *ptr, *buffer = nullptr;

	uint8_t header = CONNECT;
	uint8_t flags = 0;

	int remaining_count = 0;
	char remaining_length[4];

	//header
	header = SETQOS(header, MQTT_QOS1);
	
	//flags
	flags = FLAG_CLEANSESS(flags, this->cleansess);
	flags = FLAG_WILL(flags, (this->will) ? 1 : 0);
	if (this->will) {
		flags = FLAG_WILLQOS(flags, this->will->qos);
		flags = FLAG_WILLRETAIN(flags, this->will->retain);
	}
	if (!this->username.empty()) flags = FLAG_USERNAME(flags, 1);
	if (!this->password.empty()) flags = FLAG_PASSWD(flags, 1);

	//length
	if (!this->clientid.empty()) {
		len = 12 + 2 + this->clientid.size();
	}
	if (this->will) {
		len += 2 + this->will->topic.size();
		len += 2 + this->will->msg.size();
	}
	if (!this->username.empty()) {
		len += 2 + this->username.size();
	}
	if (!this->password.empty()) {
		len += 2 + this->password.size();
	}
	
	remaining_count = _encode_remaining_length(remaining_length, len);

	ptr = buffer = (char *)alloca(1 + remaining_count + len);
	
	_write_header(&ptr, header);
	_write_remaining_length(&ptr, remaining_length, remaining_count);
	_write_string_len(&ptr, PROTOCOL_MAGIC, 6);
	_write_char(&ptr, MQTT_PROTO_MAJOR);
	_write_char(&ptr, flags);
	_write_int(&ptr, this->keepalive);
	_write_string(&ptr, this->clientid);

	if (this->will) {
		_write_string(&ptr, this->will->topic);
		_write_string(&ptr, this->will->msg);
	}
	if (!this->username.empty()) {
		_write_string(&ptr, this->username);
	}
	if (!this->password.empty()) {
		_write_string(&ptr, this->password);
	}

	anetWrite(this->fd, buffer, ptr - buffer);
}

int Mqtt::mqtt_connect()
{
	char server[1024] = {0};
	if (anetResolve(this->errstr, this->server.c_str(), server) != ANET_OK) {
		return -1;
	}
	int fd = anetTcpConnect(this->errstr, server, this->port);
	if (fd < 0) {
		return fd;
	}
	this->fd = fd;
	//	aeCreateFileEvent(mqtt->el, fd, AE_READABLE, (aeFileProc *)_mqtt_read, (void *)mqtt);
	_mqtt_send_connect();
	mqtt_set_state(MQTT_STATE_CONNECTING);
	_mqtt_callback(CONNECT, nullptr, MQTT_STATE_CONNECTING);

	return fd;
}

void Mqtt::_mqtt_send_publish(MqttMsg *msg)
{
	int len = 0;
	char *ptr, *buffer;
	char remaining_length[4];
	int remaining_count;

	uint8_t header = PUBLISH;
	header = SETRETAIN(header, msg->retain);
	header = SETQOS(header, msg->qos);
	header = SETDUP(header, msg->dup);

	len += 2 + msg->topic.size();

	if (msg->qos > MQTT_QOS0) len += 2; //msgid

	len += msg->payload.size();
	
	remaining_count = _encode_remaining_length(remaining_length, len);
	
	ptr = buffer = (char *)alloca(1 + remaining_count + len);

	_write_header(&ptr, header);
	_write_remaining_length(&ptr, remaining_length, remaining_count);
	_write_string(&ptr, msg->topic);
	if (msg->qos > MQTT_QOS0) {
		_write_int(&ptr, msg->id);
	}
	if (!msg->payload.empty()) {
		_write_payload(&ptr, msg->payload);
	}
	anetWrite(this->fd, buffer, ptr-buffer);
}

//PUBLISH
int Mqtt::mqtt_publish(MqttMsg *msg)
{
	if (msg->id == 0) {
		msg->id = this->msgid++;
	}
	_mqtt_send_publish(msg);
	_mqtt_callback(PUBLISH, msg, msg->id);
	return msg->id;
}

void Mqtt::_mqtt_send_ack(int type, int msgid)
{
	char buffer[4] = {type, 2, MSB(msgid), LSB(msgid)};
	anetWrite(this->fd, buffer, 4);
}

//PUBACK for QOS_1, QOS_2
void Mqtt::mqtt_puback(int msgid)
{
	_mqtt_send_ack(PUBACK, msgid);
}

//PUBREC for QOS_2
void Mqtt::mqtt_pubrec(int msgid)
{
	_mqtt_send_ack(PUBREC, msgid);
}

//PUBREL for QOS_2
void Mqtt::mqtt_pubrel(int msgid)
{
	_mqtt_send_ack(PUBREL, msgid);
}

//PUBCOMP for QOS_2
void Mqtt::mqtt_pubcomp(int msgid)
{
	_mqtt_send_ack(PUBCOMP, msgid);
}

static void _mqtt_send_subscribe(Mqtt *mqtt, int msgid, const char *topic, uint8_t qos)
{

	int len = 0;
	char *ptr, *buffer;

	int remaining_count;
	char remaining_length[4];

	uint8_t header = SETQOS(SUBSCRIBE, MQTT_QOS1);

	len += 2; //msgid
	len += 2 + strlen(topic) + 1; //topic and qos

	remaining_count = _encode_remaining_length(remaining_length, len);
	ptr = buffer = (char *)alloca(1 + remaining_count + len);
	
	_write_header(&ptr, header);
	_write_remaining_length(&ptr, remaining_length, remaining_count);
	_write_int(&ptr, msgid);
	_write_string(&ptr, topic);
	_write_char(&ptr, qos);

	anetWrite(mqtt->fd, buffer, ptr-buffer);
}

//SUBSCRIBE
int Mqtt::mqtt_subscribe(const char *topic, unsigned char qos)
{
	int msgid = this->msgid++;
	_mqtt_send_subscribe(this, msgid, topic, qos);
	_mqtt_callback(SUBSCRIBE, (void *)topic, msgid);
	return msgid;
}

void Mqtt::_mqtt_send_unsubscribe(int msgid, std::string const &topic)
{
	int len = 0;
	char *ptr, *buffer;
	
	int remaining_count;
	char remaining_length[4];

	uint8_t header = SETQOS(UNSUBSCRIBE, MQTT_QOS1);
	
	len += 2; //msgid
	len += 2 + topic.size(); //topic

	remaining_count = _encode_remaining_length(remaining_length, len);
	ptr = buffer = (char *)alloca(1 + remaining_count + len);
	
	_write_header(&ptr, header);
	_write_remaining_length(&ptr, remaining_length, remaining_count);
	_write_int(&ptr, msgid);
	_write_string(&ptr, topic);

	anetWrite(this->fd, buffer, ptr-buffer);
}

//UNSUBSCRIBE
int Mqtt::mqtt_unsubscribe(std::string const &topic)
{
	int msgid = this->msgid++;
	_mqtt_send_unsubscribe(msgid, topic);
	_mqtt_callback(UNSUBSCRIBE, (void *)topic.c_str(), msgid);
	return msgid;
}

void Mqtt::_mqtt_send_ping()
{
	char buffer[2] = {(char)PINGREQ, 0};
	anetWrite(this->fd, buffer, 2);
}

//PINGREQ
void Mqtt::mqtt_ping()
{
	_mqtt_send_ping();
	_mqtt_callback(PINGREQ, nullptr, 0);
}

static void _mqtt_send_disconnect(Mqtt *mqtt)
{
	char buffer[2] = {(char)DISCONNECT, 0};
	anetWrite(mqtt->fd, buffer, 2);
}

//DISCONNECT
void Mqtt::mqtt_disconnect()
{
	_mqtt_send_disconnect(this);
	if (this->fd > 0) {
		::close(this->fd);
		this->fd = -1;
	}
	mqtt_set_state(MQTT_STATE_DISCONNECTED);
	_mqtt_callback(CONNECT, nullptr, MQTT_STATE_DISCONNECTED);
}

static void _mqtt_sleep(struct aeEventLoop *evtloop)
{
	MQTT_NOTUSED(evtloop);
	//what to do?
}

void Mqtt::close()
{
	this->will.reset();
}

int Mqtt::_mqtt_keepalive(long long id, void *clientdata)
{
	//	assert(el);
	MQTT_NOTUSED(id);
	Mqtt *mqtt = (Mqtt *)clientdata;
	mqtt->_mqtt_send_ping();
	mqtt->_mqtt_callback(PINGREQ, nullptr, 0);
	//FIXME: TIMEOUT
	//mqtt->keepalive->timeoutid = aeCreateTimeEvent(el,
	//   period*2, mqtt_keepalive_timeout, mqtt, nullptr);
	return mqtt->keepalive*1000;
}

/*--------------------------------------
** MQTT handler and reader.
--------------------------------------*/
void Mqtt::_mqtt_handle_connack(int rc)
{
	_mqtt_callback(CONNACK, nullptr, rc);
	if (rc == CONNACK_ACCEPT) {
		mqtt_set_state(MQTT_STATE_CONNECTED);
		_mqtt_callback(CONNECT, nullptr, MQTT_STATE_CONNECTED);
	}
}

void Mqtt::_mqtt_handle_publish(MqttMsg *msg)
{
	if (msg->qos == MQTT_QOS1) {
		mqtt_puback(msg->id);
	} else if (msg->qos == MQTT_QOS2) {
		mqtt_pubrec(msg->id);
	}
	_mqtt_msg_callback(this, msg);
}

void Mqtt::_mqtt_handle_puback(int type, int msgid)
{
	if (type == PUBREL) {
		mqtt_pubcomp(msgid);
	}
	_mqtt_callback(type, nullptr, msgid);
}

void Mqtt::_mqtt_handle_suback(int msgid, int qos)
{
	MQTT_NOTUSED(qos);
	_mqtt_callback(SUBACK, nullptr, msgid);
}

void Mqtt::_mqtt_handle_unsuback(int msgid)
{
	_mqtt_callback(UNSUBACK, nullptr, msgid);
}

void Mqtt::_mqtt_handle_pingresp()
{
	_mqtt_callback(PINGRESP, nullptr, 0);
}

void Mqtt::_mqtt_handle_publish(uint8_t header, char *buffer, int buflen)
{
	std::vector<char> payload;
	int qos = GETQOS(header);
	bool retain = GETRETAIN(header);
	bool dup = GETDUP(header);
	std::string topic = _read_string_len(&buffer);
	int payloadlen = buflen;
	payloadlen -= 2;
	payloadlen -= topic.size();
	if (qos > 0) {
		msgid = _read_int(&buffer);
		payloadlen -= 2;
	}
	payload.resize(payloadlen + 1);
	memcpy(payload.data(), buffer, payloadlen);
	payload[payloadlen] = 0;
	MqttMsg msg;
	mqtt_msg_new(&msg, msgid, qos, retain, dup, topic, payload.data(), payload.size());
	this->_mqtt_handle_publish(&msg);
}

void Mqtt::_mqtt_handle_packet(uint8_t header, char *buffer, int buflen)
{
	int qos, msgid = 0;
	uint8_t type = GETTYPE(header);
	switch (type) {
	case CONNACK:
		_read_char(&buffer);
		_mqtt_handle_connack(_read_char(&buffer));
		break;
	case PUBLISH:
		_mqtt_handle_publish(header, buffer, buflen);
		break;
	case PUBACK:
	case PUBREC:
	case PUBREL:
	case PUBCOMP:
		msgid = _read_int(&buffer);
		_mqtt_handle_puback(type, msgid);
		break;
	case SUBACK:
		msgid = _read_int(&buffer);
		qos = _read_char(&buffer);
		_mqtt_handle_suback(msgid, qos);
		break;
	case UNSUBACK:
		msgid = _read_int(&buffer);
		_mqtt_handle_unsuback(msgid);
		break;
	case PINGRESP:
		_mqtt_handle_pingresp();
		break;
	default:
		_mqtt_set_error(this->errstr, "badheader: %d", type);
	}
}

void Mqtt::_mqtt_reader_feed(char *buffer, int len)
{
	uint8_t header;
	char *ptr = buffer;
	int remaining_length;
	int remaining_count;
	
	header = _read_header(&ptr);
	
	remaining_length = _decode_remaining_length(&ptr, &remaining_count);
	if (1 + remaining_count+remaining_length != len) {
		_mqtt_set_error(this->errstr, "badpacket: remaing_length=%d, remaing_count=%d, len=%d", remaining_length, remaining_count, len);
		return;
	}
	_mqtt_handle_packet(header, ptr, remaining_length);
}

void Mqtt::mqtt_read(int fd, int mask)
{
	int nread;
	char buffer[MQTT_BUFFER_SIZE];

	MQTT_NOTUSED(mask);

	nread = read(fd, buffer, MQTT_BUFFER_SIZE);
	if (nread < 0) {
		if (errno == EAGAIN) {
			return;
		} else {
			this->error = errno;
			_mqtt_set_error(this->errstr, "socket error: %d.", errno);
		}
	} else if (nread == 0) {
		mqtt_disconnect();
	} else {
		_mqtt_reader_feed(buffer, nread);
	}
}

std::shared_ptr<MqttWill> mqtt_will_new(std::string const &topic, std::string const &msg, bool retain, uint8_t qos)
{
	std::shared_ptr<MqttWill> will = std::make_shared<MqttWill>();
	will->topic = topic;
	will->msg = msg;
	will->retain = retain;
	will->qos = qos;
	return will;
}

void mqtt_msg_new(MqttMsg *msg, int msgid, int qos, bool retain, bool dup, std::string const &topic, char const *ptr, size_t len)
{
	*msg = {};
	msg->id = msgid;
	msg->qos = qos;
	msg->retain = retain;
	msg->dup = dup;
	msg->topic = topic;
	msg->payload.assign(ptr, ptr + len);
}

static const char *msg_names[] = {
	"RESERVED",
	"CONNECT",
	"CONNACK",
	"PUBLISH",
	"PUBACK",
	"PUBREC",
	"PUBREL",
	"PUBCOMP",
	"SUBSCRIBE",
	"SUBACK",
	"UNSUBSCRIBE",
	"UNSUBACK",
	"PINGREQ",
	"PINGRESP",
	"DISCONNECT"
};

const char *Mqtt::mqtt_msg_name(uint8_t type)
{
	type = (type >> 4) & 0x0F;
	return (type >= 0 && type <= DISCONNECT) ? msg_names[type] : "UNKNOWN";
}

