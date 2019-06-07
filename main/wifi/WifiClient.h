#ifndef WIFICLIENT_H_
#define WIFICLIENT_H_

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"

typedef struct wifi_client_s
{
	char	*ssid;
	char	*password;
	EventGroupHandle_t	eventGroup;
}				wifi_client_t;


void taskWifi(void *arg);

#endif // !WIFICLIENT_H_