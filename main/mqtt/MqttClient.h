#ifndef MQTTCLIENT_H_
#define MQTTCLIENT_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"

typedef struct MqttConfig_s
{
	char	*url;
}				MqttConfig_t;

esp_err_t	startMqttClient(MqttConfig_t *config);
esp_err_t	stopMqttClient();

#endif // !MQTTCLIENT_H_