#ifndef MQTTCLIENT_H_
#define MQTTCLIENT_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "mqtt_client.h"
#include "freertos/event_groups.h"

#include "esp_log.h"

esp_err_t    launchMqtt(EventGroupHandle_t eventGroup, const char *url);

#endif // !MQTTCLIENT_H_