
#include "client.h"
#include "../mqttserver.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>

//extern Client client;


void client_prepare();
void set_callbacks(Mqtt *mqtt);

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

	client.mqtt->mqtt_subscribe(MQTT_TOPIC, 0);

	std::mutex mutex;
	std::condition_variable cond;
	bool interrupted = false;

	std::thread ping_thread([&](){
		while (1) {
			{
				std::unique_lock<std::mutex> lock(mutex);
				cond.wait_for(lock, std::chrono::seconds(30));
				if (interrupted) break;
			}
			client.mqtt->mqtt_ping();
		}
	});


	while (1) {
		client.mqtt->mqtt_read(client.mqtt->fd, 0);
	}

	return 0;
}

