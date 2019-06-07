
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"

#include "wifi/WifiClient.h"
#include "mqtt/MqttClient.h"
#include "sensors/SensorClient.h"

static const char	TAG[] = "Main";

void	app_main() {
    WifiConfig_t	dataWifi = {
        .ssid = "Honor Raphael",
        .password = "clemon69"
    };
    MqttConfig_t	dataMqtt = {
        .url = "mqtt://iot.eclipse.org"
    };

    nvs_flash_init();
	startWifiClient(&dataWifi);
	startMqttClient(&dataMqtt);
	startSensorClient();
}