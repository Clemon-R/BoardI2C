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
#include "freertos/queue.h"

#include "esp_log.h"

#include "../ClientState.h"
#include "../Constant.h"

typedef struct MqttConfig_s {
    uint8_t	*url;
    uint16_t    port;
}				MqttConfig_t;

typedef enum AlertType_e
{
    INFO = 0,
    MINOR,
    MAJOR
}           AlertType_t;

char	isMqttConnected();

esp_err_t	startMqttClient(MqttConfig_t *config);
esp_err_t	stopMqttClient();
char    mqttIsRunning();
void    restartMqttClient();

QueueHandle_t	getQueueDatas();

BaseType_t    createAlert(const char *description, const AlertType_t type, const char closed);

#endif // !MQTTCLIENT_H_