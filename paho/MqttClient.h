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

#ifndef AMAZONALEXA_H
#define AMAZONALEXA_H


class MqttClient {
private:
	int mysock = -1;
public:
	static int transport_sendPacketBuffer(int sock, unsigned char *buf, int buflen, void *opaque);
	static int transport_getdata(unsigned char *buf, int count, void *opaque);
	static int transport_getdatanb(void *sck, unsigned char *buf, int count);
	static int transport_open(char *host, int port, void *opaque);
	static int transport_close(int sock);
	int publish();
	int subscribe();
	int close();
};

#endif // AMAZONALEXA_H
