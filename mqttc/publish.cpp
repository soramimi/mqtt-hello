
#include "client.h"
#include "../mqttserver.h"
#include <cstring>


extern int connack;

void set_callbacks(Mqtt *mqtt);
void client_init();

int main()
{
	Client client;
	client.init();

	client.mqtt->mqtt_set_server(MQTT_SERVER);
	client.mqtt->mqtt_set_username(MQTT_USERNAME);

	client.set_callbacks();
	
	if (client.mqtt->mqtt_connect() < 0) {
		printf("mqttc connect failed.\n");
		exit(-1);
	}

	while (client.mqtt->connack == 0) {
		client.mqtt->mqtt_read(client.mqtt->fd, 0);
	}

	char const *m = "Hello, world";
	MqttMsg msg;
	mqtt_msg_new(&msg, 0, 0, false, false, MQTT_TOPIC, m, strlen(m));
	client.mqtt->mqtt_publish(&msg);

	return 0;
}

