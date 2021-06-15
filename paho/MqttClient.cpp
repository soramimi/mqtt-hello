/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial API and implementation and/or initial documentation
 *    Sergio R. Caprile - "commonalization" from prior samples and/or documentation extension
 *******************************************************************************/

#include "MqttClient.h"

#include <sys/types.h>

#if !defined(SOCKET_ERROR)
/** error in socket operation */
#define SOCKET_ERROR -1
#endif

#if defined(WIN32)
/* default on Windows is 64 - increase to make Linux and Windows the same */
#define FD_SETSIZE 1024
#include <winsock2.h>
#include <ws2tcpip.h>
#define MAXHOSTNAMELEN 256
#define EAGAIN WSAEWOULDBLOCK
#define EINTR WSAEINTR
#define EINVAL WSAEINVAL
#define EINPROGRESS WSAEINPROGRESS
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOTCONN WSAENOTCONN
#define ECONNRESET WSAECONNRESET
#define ioctl ioctlsocket
#define socklen_t int
#else
#define INVALID_SOCKET SOCKET_ERROR
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#endif

#include "MQTTPacket.h"

#if defined(WIN32)
#include <Iphlpapi.h>
#else
#include <sys/ioctl.h>
#include <net/if.h>
#endif

#include "../mqttserver.h"

/**
This simple low-level implementation assumes a single connection for a single thread. Thus, a static
variable is used for that connection.
On other scenarios, the user must solve this by taking into account that the current implementation of
MQTTPacket_read() has a function pointer for a function call to get the data to a buffer, but no provisions
to know the caller or other indicator (the socket id): int (*getfn)(unsigned char*, int)
*/
//static int mysock = INVALID_SOCKET;


int MqttClient::transport_sendPacketBuffer(int sock, unsigned char *buf, int buflen, void *opaque)
{
	int rc = 0;
	rc = write(sock, buf, buflen);
	return rc;
}


int MqttClient::transport_getdata(unsigned char *buf, int count, void *opaque)
{
	MqttClient *self = reinterpret_cast<MqttClient *>(opaque);
	int rc = recv(self->mysock, buf, count, 0);
	//printf("received %d bytes count %d\n", rc, (int)count);
	return rc;
}

int MqttClient::transport_getdatanb(void *sck, unsigned char *buf, int count)
{
	int sock = *((int *)sck); 	/* sck: pointer to whatever the system may use to identify the transport */
	/* this call will return after the timeout set on initialization if no bytes;
	   in your system you will use whatever you use to get whichever outstanding
	   bytes your socket equivalent has ready to be extracted right now, if any,
	   or return immediately */
	int rc = recv(sock, buf, count, 0);
	if (rc == -1) {
		/* check error conditions from your system here, and return -1 */
		return 0;
	}
	return rc;
}

/**
return >=0 for a socket descriptor, <0 for an error code
@todo Basically moved from the sample without changes, should accomodate same usage for 'sock' for clarity,
removing indirections
*/
int MqttClient::transport_open(char *addr, int port, void *opaque)
{
	int sock = -1;
	int type = SOCK_STREAM;
	struct sockaddr_in address;
#if defined(AF_INET6)
	struct sockaddr_in6 address6;
#endif
	int rc = -1;
#if defined(WIN32)
	short family;
#else
	sa_family_t family = AF_INET;
#endif
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};
	static struct timeval tv;

	sock = -1;
	if (addr[0] == '[')
		++addr;

	rc = getaddrinfo(addr, NULL, &hints, &result);
	if (rc == 0) {
		struct addrinfo* res = result;

		/* prefer ip4 addresses */
		while (res) {
			if (res->ai_family == AF_INET) {
				result = res;
				break;
			}
			res = res->ai_next;
		}

#if defined(AF_INET6)
		if (result->ai_family == AF_INET6) {
			address6.sin6_port = htons(port);
			address6.sin6_family = family = AF_INET6;
			address6.sin6_addr = ((struct sockaddr_in6*)(result->ai_addr))->sin6_addr;
		} else
#endif
			if (result->ai_family == AF_INET)
			{
				address.sin_port = htons(port);
				address.sin_family = family = AF_INET;
				address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
			} else {
				rc = -1;
			}

		freeaddrinfo(result);
	}

	if (rc == 0) {
		sock =	socket(family, type, 0);
		if (sock != -1) {
#if defined(NOSIGPIPE)
			int opt = 1;

			if (setsockopt(*sock, SOL_SOCKET, SO_NOSIGPIPE, (void*)&opt, sizeof(opt)) != 0)
				Log(TRACE_MIN, -1, "Could not set SO_NOSIGPIPE for socket %d", *sock);
#endif

			if (family == AF_INET)
				rc = connect(sock, (struct sockaddr*)&address, sizeof(address));
#if defined(AF_INET6)
			else
				rc = connect(sock, (struct sockaddr*)&address6, sizeof(address6));
#endif
		}
	}
	if (sock == INVALID_SOCKET) return rc;

	tv.tv_sec = 1;  /* 1 second Timeout */
	tv.tv_usec = 0;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

	MqttClient *self = reinterpret_cast<MqttClient *>(opaque);
	self->mysock = sock;
	return sock;
}

int MqttClient::transport_close(int sock)
{

	int rc;

	rc = shutdown(sock, SHUT_WR);
	rc = recv(sock, NULL, (size_t)0, 0);
	rc = ::close(sock);
	sock = -1;
	return rc;
}

int MqttClient::close()
{
	transport_close(mysock);
	mysock = -1;
}

int MqttClient::publish()
{
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	int rc = 0;
	int mysock = 0;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	MQTTString topicString = MQTTString_initializer;
	char *payload = "Hello, world";
	int payloadlen = strlen(payload);
	int len = 0;
	char *host = MQTT_SERVER;
	int port = 1883;

	mysock = MqttClient::transport_open(host, port, this);
	if (mysock < 0) return mysock;

//	printf("Sending to hostname %s port %d\n", host, port);

	data.clientID.cstring = "";
	data.keepAliveInterval = 20;
	data.cleansession = 1;
	data.username.cstring = MQTT_USERNAME;
	data.password.cstring = MQTT_PASSWORD;

	len = MQTTSerialize_connect(buf, buflen, &data);
	rc = MqttClient::transport_sendPacketBuffer(mysock, buf, len, this);

	/* wait for connack */
	if (MQTTPacket_read(buf, buflen, MqttClient::transport_getdata, this) == CONNACK) {
		unsigned char sessionPresent, connack_rc;
		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen, this) != 1 || connack_rc != 0) {
			printf("Unable to connect, return code %d\n", connack_rc);
			goto exit;
		}
	} else {
		goto exit;
	}

	topicString.cstring = MQTT_TOPIC;
	printf("publishing reading\n");
	len = MQTTSerialize_publish(buf, buflen, 0, 0, 0, 0, topicString, (unsigned char*)payload, payloadlen);
	rc = MqttClient::transport_sendPacketBuffer(mysock, buf, len, this);

	printf("disconnecting\n");
	len = MQTTSerialize_disconnect(buf, buflen);
	rc = MqttClient::transport_sendPacketBuffer(mysock, buf, len, this);

exit:
	close();
	return 0;
}

int MqttClient::subscribe()
{
	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	int rc = 0;
	int mysock = 0;
	unsigned char buf[200];
	int buflen = sizeof(buf);
	int msgid = 1;
	MQTTString topicString = MQTTString_initializer;
	int req_qos = 0;
	int len = 0;
	char *host = MQTT_SERVER;
	int port = 1883;

	mysock = MqttClient::transport_open(host, port, this);
	if (mysock < 0) return mysock;

//	printf("Sending to hostname %s port %d\n", host, port);

	data.clientID.cstring = "";
	data.keepAliveInterval = 20;
	data.cleansession = 1;
	data.username.cstring = MQTT_USERNAME;
	data.password.cstring = MQTT_PASSWORD;

	len = MQTTSerialize_connect(buf, buflen, &data);
	rc = MqttClient::transport_sendPacketBuffer(mysock, buf, len, this);

	/* wait for connack */
	if (MQTTPacket_read(buf, buflen, MqttClient::transport_getdata, this) == CONNACK) {
		unsigned char sessionPresent, connack_rc;

		if (MQTTDeserialize_connack(&sessionPresent, &connack_rc, buf, buflen, this) != 1 || connack_rc != 0) {
			printf("Unable to connect, return code %d\n", connack_rc);
			goto exit;
		}
	} else {
		goto exit;
	}

	/* subscribe */
	topicString.cstring = MQTT_TOPIC;
	len = MQTTSerialize_subscribe(buf, buflen, 0, msgid, 1, &topicString, &req_qos);

	rc = MqttClient::transport_sendPacketBuffer(mysock, buf, len, this);
	if (MQTTPacket_read(buf, buflen, MqttClient::transport_getdata, this) == SUBACK) {
		unsigned short submsgid;
		int subcount;
		int granted_qos;

		rc = MQTTDeserialize_suback(&submsgid, 1, &subcount, &granted_qos, buf, buflen, this);
		if (granted_qos != 0) {
			printf("granted qos != 0, %d\n", granted_qos);
			goto exit;
		}
	} else {
		goto exit;
	}

	/* loop getting msgs on subscribed topic */
	topicString.cstring = MQTT_TOPIC;
	while (1) {
		if (MQTTPacket_read(buf, buflen, MqttClient::transport_getdata, this) == PUBLISH) {
			unsigned char dup;
			int qos;
			unsigned char retained;
			unsigned short msgid;
			int payloadlen_in;
			unsigned char *payload_in;
			int rc;
			MQTTString receivedTopic;

			rc = MQTTDeserialize_publish(&dup, &qos, &retained, &msgid, &receivedTopic, &payload_in, &payloadlen_in, buf, buflen, this);
			printf("message arrived %.*s\n", payloadlen_in, payload_in);
		}
	}

	printf("disconnecting\n");
	len = MQTTSerialize_disconnect(buf, buflen);
	rc = MqttClient::transport_sendPacketBuffer(mysock, buf, len, this);

exit:
	close();
	return 0;
}
