
#include "client.h"
#include "../mqttserver.h"

extern Client client;

extern int connack;

void set_callbacks(Mqtt *mqtt);
void client_init();

int main()
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

	char const *m = "Hello, world";
	MqttMsg *msg = mqtt_msg_new(0, 0, false, false, strdup(MQTT_TOPIC), strlen(m), strdup(m));
	mqtt_publish(client.mqtt, msg);
	mqtt_msg_free(msg);

	return 0;
}

