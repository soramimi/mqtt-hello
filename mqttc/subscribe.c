
#include "client.h"
#include "../mqttserver.h"

extern Client client;

extern int connack;

void client_prepare();
void set_callbacks(Mqtt *mqtt);
void client_init();

int main(int argc, char **argv)
{
	client_init();

	mqtt_set_server(client.mqtt, MQTT_SERVER);
	mqtt_set_username(client.mqtt, MQTT_USERNAME);

	set_callbacks(client.mqtt);
	
	if (mqtt_connect(client.mqtt) < 0) {
		printf("mqttc connect failed.\n");
		exit(-1);
	}

	while (connack == 0) {
		_mqtt_read(client.mqtt->fd, client.mqtt, 0);
	}

	mqtt_subscribe(client.mqtt, MQTT_TOPIC, 0);
	while (1) {
		_mqtt_read(client.mqtt->fd, client.mqtt, 0);
	}

	return 0;
}

