
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
    gpio_conf.pin_bit_mask = RGB_MASK;
    gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_conf);
    gpio_set_level(RGB_1_RED, 1);
    gpio_set_level(RGB_1_GREEN, 1);
    gpio_set_level(RGB_2_RED, 1);
    gpio_set_level(RGB_2_GREEN, 1);
    gpio_set_level(RGB_3_GREEN, 1);
    gpio_set_level(RGB_3_RED, 1);
    gpio_set_level(RGB_3_BLUE, 1);
}

static void btnClicked(uint32_t io_num, TypeClick type)
{
    switch (io_num) {
    case BTN_LEFT:
        ESP_LOGI(TAG, "Left");
        if (type == Simple || type == Double) {
            previousPage();
        }
        break;

    case BTN_RIGHT:
        ESP_LOGI(TAG, "Right");
        if (type == Simple || type == Double) {
            nextPage();
        }
        break;

    case BTN_TOP:
        ESP_LOGI(TAG, "Top");
        if (getCurrentPage() == 0 && (type == Simple || type == Double)){
            nextSensorsPage();
        }
        break;

    case BTN_BOTTOM:
        ESP_LOGI(TAG, "Bottom");
        if (getCurrentPage() == 0 && (type == Simple || type == Double)){
            previousSensorsPage();
        }
        break;

    case BTN_CLICK:
        ESP_LOGI(TAG, "Click");
        break;
    
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
    srand(time(NULL));
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