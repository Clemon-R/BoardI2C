#ifndef BLESERVER_H_
#define	BLESERVER_H_

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

#define	SERVICE_NUM 3
#define SERVICE_WIFI   		0x00FF
#define SERVICE_MQTT   		0x00FE
#define SERVICE_SENSORS   	0x00FD

#define	CHAR_NUM					12
#define WIFI_CHAR_OFFSET				0
#define	WIFI_CHAR_SSID					0xFF01
#define	WIFI_CHAR_PASSWORD				0xFF02
#define	WIFI_CHAR_ACTION				0xFF03
#define MQTT_CHAR_OFFSET				3
#define	MQTT_CHAR_URL					0xFE01
#define	MQTT_CHAR_PORT					0xFE02
#define	MQTT_CHAR_ACTION				0xFE03
#define SENSORS_CHAR_OFFSET				6
#define	SENSORS_CHAR_STATE				0xFD01
#define	SENSORS_CHAR_DELAI				0xFD02
#define	SENSORS_CHAR_TEMP				0xFD03
#define	SENSORS_CHAR_HUMIDITY			0xFD04
#define	SENSORS_CHAR_PRESSURE			0xFD05
#define	SENSORS_CHAR_COLOR				0xFD06

#define GATTS_NUM_HANDLE_TEST_A	14

#define DEVICE_NAME            "Demonstrator"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 1

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)


#ifndef	BUFF_SIZE
#define	BUFF_SIZE 512
#endif

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