
#include <stdio.h>
#include <mosquitto.h>
#include "../tlskeys.h"

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
	if (qos >= 0 && qos <= 2) return 1;

	fprintf(stderr, "%d: %s out of range\n", qos, type);
	return 0;
}

int main()
{
	int rc = 1;
	struct userdata ud;
	struct mosquitto *mosq = NULL;

	memset(&ud, 0, sizeof(ud));

	ud.topic_count++;
	ud.topics = realloc(ud.topics, sizeof(char *) * ud.topic_count);
	ud.topics[ud.topic_count - 1] = MQTT_TOPIC;

	mosquitto_lib_init();
	mosq = mosquitto_new(NULL, true, &ud);
	if (!mosq) {
		return perror_ret("mosquitto_new");
	}

	mosquitto_tls_set(mosq, cafile, NULL, crtfile, keyfile, NULL);
	mosquitto_tls_opts_set(mosq, 1, NULL, NULL);

	mosquitto_connect_callback_set(mosq, connect_cb);
	mosquitto_message_callback_set(mosq, message_cb);

	rc = mosquitto_connect(mosq, MQTT_SERVER, MQTT_PORT, 60);
	if (rc != MOSQ_ERR_SUCCESS) {
		if (rc == MOSQ_ERR_ERRNO) {
			return perror_ret("mosquitto_connect_bind");
		}
		fprintf(stderr, "Unable to connect (%d)\n", rc);
		goto cleanup;
	}

	rc = mosquitto_loop_forever(mosq, -1, 1);

cleanup:
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return rc;

}
