
#include <mosquitto.h>
#include <stdio.h>
#include <sched.h>
#include "../tlskeys.h"

int connected = 0;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if (reason_code != 0){
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
	if (!mosq) {
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	mosquitto_tls_set(mosq, cafile, NULL, crtfile, keyfile, NULL);
	mosquitto_tls_opts_set(mosq, 1, NULL, NULL);

	mosquitto_connect_callback_set(mosq, on_connect);

	rc = mosquitto_connect(mosq, MQTT_SERVER, MQTT_PORT, 60);
	if (rc != MOSQ_ERR_SUCCESS) {
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

	rc = mosquitto_loop_start(mosq);
	if (rc != MOSQ_ERR_SUCCESS) {
		mosquitto_destroy(mosq);
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return 1;
	}

	while (!connected) {
		sched_yield();
	}

	char const *payload = "Hello, world";
	rc = mosquitto_publish(mosq, NULL, MQTT_TOPIC, strlen(payload), payload, 0, false);
	if (rc != MOSQ_ERR_SUCCESS) {
		fprintf(stderr, "Error publishing: %s\n", mosquitto_strerror(rc));
	}

	mosquitto_lib_cleanup();
	return 0;
}
