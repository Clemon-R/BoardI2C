#ifndef BLESERVER_H_
#define	BLESERVER_H_

#include "../../demo_conf.h"

#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
//#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "../wifi/WifiClient.h"
#include "../mqtt/MqttClient.h"



#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

typedef enum	BleAction_e
{
	RESTART_WIFI = 0
}				BleAction_t;

typedef struct	BleServerConfig_s
{
	WifiConfig_t	*wifiConfig;
	MqttConfig_t	*mqttConfig;
}				BleServerConfig_t;

esp_err_t	startBleServer(BleServerConfig_t *config);
esp_err_t	stopBleServer();

#endif