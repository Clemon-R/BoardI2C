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

typedef struct mqtt_client_s
{
	char	*url;
	EventGroupHandle_t	eventGroup;
}				mqtt_client_t;

void	taskMqtt(void *arg);

#endif // !MQTTCLIENT_H_