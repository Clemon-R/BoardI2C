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


#define SERVICE_WIFI_CONFIGURATOR   0x00FF
#define	CHAR_NUM					3
#define	CHAR_SSID					0xFF01
#define	CHAR_PASSWORD				0xFF02
#define	CHAR_ACTION					0xFF03
#define GATTS_DESCR_UUID_TEST_A     0x3333
#define GATTS_NUM_HANDLE_TEST_A     8

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
}				BleServerConfig_t;

esp_err_t	startBleServer(BleServerConfig_t *config);
esp_err_t	stopBleServer();

#endif