/* 
 * packet.h - mqtt packet encode and decode header
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
#ifndef __MQTT_PACKET_H
#define __MQTT_PACKET_H

#include <stdint.h>
#include <stdbool.h>

#define PROTOCOL_MAGIC "MQIsdp"

#define CONNECT 0x10
#define CONNACK 0x20
#define PUBLISH 0x30
#define PUBACK 0x40
#define PUBREC 0x50
#define PUBREL 0x60
#define PUBCOMP 0x70
#define SUBSCRIBE 0x80
#define SUBACK 0x90
#define UNSUBSCRIBE 0xA0
#define UNSUBACK 0xB0
#define PINGREQ 0xC0
#define PINGRESP 0xD0
#define DISCONNECT 0xE0

#define LSB(A) (uint8_t)(A & 0x00FF)
#define MSB(A) (uint8_t)((A & 0xFF00) >> 8)

/*
|--------------------------------------
| 7 6 5 4 |     3    |  2 1  | 0      |
|  Type   | DUP flag |  QoS  | RETAIN |
|--------------------------------------
*/
#define GETTYPE(HDR)		(HDR & 0xF0)
#define SETQOS(HDR, Q)		(HDR | ((Q) << 1))
#define GETQOS(HDR)			((HDR & 0x06) >> 1)
#define SETDUP(HDR, D)		(HDR | ((D) << 3))
#define GETDUP(HDR)			((HDR & 0x08) >> 3)
#define SETRETAIN(HDR, R)	(HDR | (R))
#define GETRETAIN(HDR)		(HDR & 0x01)

/*
|----------------------------------------------------------------------------------
|     7    |    6     |      5     |  4   3  |     2    |       1      |     0    |
| username | password | willretain | willqos | willflag | cleansession | reserved |
|----------------------------------------------------------------------------------
*/
#define FLAG_CLEANSESS(F, C)	(F | ((C) << 1))
#define FLAG_WILL(F, W)			(F | ((W) << 2))	
#define FLAG_WILLQOS(F, Q)		(F | ((Q) << 3))
#define FLAG_WILLRETAIN(F, R) 	(F | ((R) << 5))
#define FLAG_PASSWD(F, P)		(F | ((P) << 6))
#define FLAG_USERNAME(F, U)		(F | ((U) << 7))

#define MAX_PAYLOAD_SIZE 268435455

int _encode_remaining_length(char *buf, int length);

int _decode_remaining_length(char **buf, int *count);

void _write_header(char **pptr, uint8_t header);

uint8_t _read_header(char **pptr);

void _write_remaining_length(char **ptr, char *bytes, int count);

void _write_char(char **pptr, char c);

char _read_char(char** pptr);

void _write_int(char **pptr, int i);

int _read_int(char** pptr);

void _write_string(char **pptr, const char *string);

char *_read_string(char** pptr);

void _write_string_len(char **pptr, const char *string, int len);

char *_read_string_len(char **pptr, int *len);

void _write_payload(char **pptr, const char *payload, int length);

#endif /* __MQTT_PACKET_H */

