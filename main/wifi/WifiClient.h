#ifndef WIFICLIENT_H_
#define WIFICLIENT_H_

#include "../../demo_conf.h"

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

#include "../ClientState.h"

typedef struct WifiConfig_s {
    uint8_t	*ssid;
    uint8_t	*password;
}				WifiConfig_t;

char	isWifiConnected();

esp_err_t	startWifiClient(WifiConfig_t   *config);
esp_err_t	stopWifiClient();

EventGroupHandle_t  getWifiEventGroup();
char	wifiIsUsed();
esp_err_t    restartWifiClient(WifiConfig_t *config);

esp_err_t   getSaveWifiConfig(WifiConfig_t *config);
esp_err_t   saveWifiConfig(WifiConfig_t *config);

#endif // !WIFICLIENT_H_