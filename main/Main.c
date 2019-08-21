
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/gpio.h"

#include "Main.h"
#include "wifi/WifiClient.h"
#include "mqtt/MqttClient.h"
#include "sensors/SensorClient.h"
#include "lcd/Lcd.h"
#include "../drv/disp_spi.h"
#include "../drv/ili9341.h"
#include "ButtonClient.h"
#include "../../lvgl/lvgl.h"
#include "esp_freertos_hooks.h"
#include <esp_ota_ops.h>

#include "ble/BleServer.h"

static const char	TAG[] = "\033[1;39;100mMain\033[0m";

static uint8_t  _chipid[6];

uint8_t *getMacAddress()
{
    return _chipid;
}

static void initLedsGpio()
{
    gpio_config_t	gpio_conf;

    ESP_LOGI(TAG, "Settings GPIO Leds...");
    gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_conf.pin_bit_mask = RGB_MASK;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_conf);
    gpio_set_level(RGB_1_RED, RGB_OFF);
    gpio_set_level(RGB_1_GREEN, RGB_OFF);
    gpio_set_level(RGB_2_RED, RGB_OFF);
    gpio_set_level(RGB_2_GREEN, RGB_OFF);
    gpio_set_level(RGB_3_GREEN, RGB_OFF);
    gpio_set_level(RGB_3_RED, RGB_OFF);
    gpio_set_level(RGB_3_BLUE, RGB_OFF);
}

static void btnHandler(uint32_t io_num, TypeClick type)
{
    if (type == Simple && !lcdIsRunning()){
        startLcd();
        return;
    }
    switch (io_num) {
    case BTN_LEFT:
        if (type == Simple || type == Double) {
            previousPage();
        }
        break;

    case BTN_RIGHT:
        if (type == Simple || type == Double) {
            nextPage();
        }
        break;

    case BTN_TOP:
        if (getCurrentPage() == 0 && (type == Simple || type == Double)){
            nextSensorsPage();
        }
        break;

    case BTN_BOTTOM:
        if (getCurrentPage() == 0 && (type == Simple || type == Double)){
            previousSensorsPage();
        }
        break;

    case BTN_CLICK:
        break;    
    }
    if (type == VeryLong && lcdIsRunning()){
        stopLcd();
    }
}

void	app_main()
{
    WifiConfig_t	dataWifi = {
        .ssid = (uint8_t *)strdup(DEFAULT_WIFI_SSID), //If changed
        .password = (uint8_t *)strdup(DEFAULT_WIFI_PASSWORD)
    };
    MqttConfig_t	dataMqtt = {
        .url = (uint8_t *)strdup(DEFAULT_MQTT_URL),
        .port = DEFAULT_MQTT_PORT
    };
    BleServerConfig_t   bleConfig = {
        .wifiConfig = &dataWifi,
        .mqttConfig = &dataMqtt
    };
    esp_err_t   ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    srand(time(NULL));
    ESP_ERROR_CHECK(esp_efuse_mac_get_default(_chipid));

    //Changing config by the saved one
    getSaveWifiConfig(&dataWifi);
    getSaveMqttConfig(&dataMqtt);
    //vTaskGetRunTimeStats()

    //Buttons
    setButtonCallback(&btnHandler);   
    startButtonClient(); //Starting button here to avoid crash when starting LCD

    //Leds
    initLedsGpio();

    //Cloud
    startWifiClient(&dataWifi);
    startMqttClient(&dataMqtt);

    //Data
    startSensorClient();
    
    //Manager/Tools
    startBleServer(&bleConfig);
    createAlert("demo_start", "The demonstrator is starting", INFO, true);
    //createStatus();

    //Lcd required functions
    //Not working without launching both in main
    lv_init(); 
    disp_spi_init(); 
    while (1) {
        if (lcdGetSemaphore() == NULL || xSemaphoreTake(lcdGetSemaphore(), 15) == pdTRUE) {
            lv_task_handler(); //LittlevGL special task
            if (lcdGetSemaphore() != NULL)
                xSemaphoreGive(lcdGetSemaphore());
        }
        vTaskDelay(1);
    }
}