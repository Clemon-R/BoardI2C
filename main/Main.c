
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
#include "ButtonClient.h"
#include "../../lvgl/lvgl.h"

#include "ble/BleServer.h"

static const char	TAG[] = "\033[1;39;100mMain\033[0m";

static void setupLeds()
{
    gpio_config_t	gpio_conf;

    ESP_LOGI(TAG, "Settings GPIO Leds...");
    gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
    gpio_conf.pin_bit_mask = (1ULL<<RGB_1) | (1ULL<<RGB_2) | (1ULL<<RGB_3);
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_conf);
    gpio_set_level(RGB_1, 0);
    gpio_set_level(RGB_2, 0);
    gpio_set_level(RGB_3, 0);
}

static void btnClicked(uint32_t io_num, TypeClick type)
{
    if (type == Simple && !lcdIsRunning()) {
        startLcd();
    }
    switch (io_num) {
    case BTN_1:
        if (type == Simple) {
            previousPage();
        }
        if (type == Double) {
            previousPage();
        }
        break;

    case BTN_2:
        if (type == Simple) {
            nextPage();
        }
        if (type == Double) {
            nextPage();
        }
        break;
    }
    if (type == VeryLong) {
        stopLcd();
    }
}

void	app_main()
{
    WifiConfig_t	dataWifi = {
        //.ssid = "1234-6789-12345",
        //.password = "12345678"
        .ssid = (uint8_t *)strdup("Honor Raphael"), //If changed
        .password = (uint8_t *)strdup("clemon69")
    };
    MqttConfig_t	dataMqtt = {
        .url = "tcp://iot.eclipse.org"
    };
    BleServerConfig_t   bleConfig = {
        .wifiConfig = &dataWifi
    };
    esp_err_t   ret;

    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    setupLeds();
    setButtonCallback(&btnClicked);

    startWifiClient(&dataWifi);
    startMqttClient(&dataMqtt);
    startLcd();
    startSensorClient();
    startBleServer(&bleConfig);
    while (1) {
        if (xSemaphoreTake(lcdGetSemaphore(), pdMS_TO_TICKS(100)) == pdTRUE) {
            lv_task_handler();
            xSemaphoreGive(lcdGetSemaphore());
            vTaskDelay(1);
        }
    }
}