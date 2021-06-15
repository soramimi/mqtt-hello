
#include <err.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mosquitto.h>

#include "../mqttserver.h"

struct userdata {
	char **topics;
	size_t topic_count;
	int qos;
};

void log_cb(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}

void message_cb(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	struct userdata *ud = (struct userdata *)obj;
	if (msg->payloadlen > 0) {
		puts(msg->payload);
	}
}

void connect_cb(struct mosquitto *mosq, void *obj, int result)
{
	struct userdata *ud = (struct userdata *)obj;
	fflush(stderr);
	if (result == 0) {
		size_t i;
		for (i = 0; i < ud->topic_count; i++) {
			mosquitto_subscribe(mosq, NULL, ud->topics[i], ud->qos);
		}
	} else {
		fprintf(stderr, "%s\n", mosquitto_connack_string(result));
	}
}

static int perror_ret(const char *msg)
{
	perror(msg);
	return 1;
}

static int valid_qos_range(int qos, const char *type)
{
	if (qos >= 0 && qos <= 2)
		return 1;

	fprintf(stderr, "%d: %s out of range\n", qos, type);
	return 0;
}

int main(int argc, char *argv[])
{
	int debug = 0;
	bool clean_session = true;
	const char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
	int i, c, rc = 1;
	struct userdata ud;
	char *id = NULL;
	struct mosquitto *mosq = NULL;
	char *username = NULL;
	char *password = NULL;

	char *will_payload = NULL;
	int will_qos = 0;
	bool will_retain = false;
	char *will_topic = NULL;

	memset(&ud, 0, sizeof(ud));

	host = MQTT_SERVER;
	ud.topic_count++;
	ud.topics = realloc(ud.topics, sizeof(char *) * ud.topic_count);
	ud.topics[ud.topic_count - 1] = MQTT_TOPIC;

	username = MQTT_USERNAME;
	password = MQTT_PASSWORD;

	mosquitto_lib_init();
	mosq = mosquitto_new(id, clean_session, &ud);
	if (mosq == NULL) {
		return perror_ret("mosquitto_new");
	}

	if (will_topic && mosquitto_will_set(mosq, will_topic, will_payload ? strlen(will_payload) : 0, will_payload, will_qos, will_retain)) {
		fprintf(stderr, "Failed to set will\n");
		goto cleanup;
	}

	if (username && !password)
		password = getenv("MQTT_EXEC_PASSWORD");

	if (!username != !password) {
		fprintf(stderr, "Need to set both username and password\n");
		goto cleanup;
	}

	if (username && password)
		mosquitto_username_pw_set(mosq, username, password);

	mosquitto_connect_callback_set(mosq, connect_cb);
	mosquitto_message_callback_set(mosq, message_cb);

	/* let kernel reap the children */
	signal(SIGCHLD, SIG_IGN);

	rc = mosquitto_connect(mosq, host, port, keepalive);
	if (rc != MOSQ_ERR_SUCCESS) {
		if (rc == MOSQ_ERR_ERRNO)
			return perror_ret("mosquitto_connect_bind");
		fprintf(stderr, "Unable to connect (%d)\n", rc);
		goto cleanup;
	}

	rc = mosquitto_loop_forever(mosq, -1, 1);

cleanup:
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return rc;

}
