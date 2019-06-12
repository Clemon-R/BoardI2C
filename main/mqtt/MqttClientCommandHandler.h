#ifndef	MQTTCLIENTCOMMANDHANDLER_H_
#define	MQTTCLIENTCOMMANDHANDLER_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "cJSON.h"

typedef enum	MqttClientCommandType_e
{
	CHANGEWIFI = 0
}				MqttClientCommandType;

typedef struct MqttClientCommand_s
{
	MqttClientCommandType	type;
	cJSON	*data;
}				MqttClientCommand_t;

esp_err_t	launchCommand(MqttClientCommand_t *command);

#endif	//!MQTTCLIENTCOMMANDHANDLER_H_