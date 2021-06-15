
#include <mosquitto.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>

#include "../mqttserver.h"

int connected = 0;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if (reason_code != 0){
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		mosquitto_disconnect(mosq);
	}

	connected = 1;
}

int main()
{
	struct mosquitto *mosq;
	int rc;

	mosquitto_lib_init();

	mosq = mosquitto_new(NULL, true, NULL);
	if (mosq == NULL){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	mosquitto_username_pw_set(mosq, MQTT_USERNAME, MQTT_PASSWORD);

	mosquitto_connect_callback_set(mosq, on_connect);

	rc = mosquitto_connect(mosq, MQTT_SERVER, 1883, 60);
	if(rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

	rc = mosquitto_loop_start(mosq);
	if (rc != MOSQ_ERR_SUCCESS){
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

	while (!connected) {
		sched_yield();
	}

	char const *payload = "Hello, world";
	rc = mosquitto_publish(mosq, NULL, MQTT_TOPIC, strlen(payload), payload, 2, false);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
	}

	mosquitto_lib_cleanup();
	return 0;
}
