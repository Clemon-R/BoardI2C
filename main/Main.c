
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

static const char	TAG[] = "\033[1;39;100mMain\033[0m";


static void setupLeds()
{
    gpio_config_t	gpio_conf;

    ESP_LOGI(TAG, "Settings GPIO Leds...");
	gpio_conf.intr_type = GPIO_INTR_ANYEDGE;
	gpio_conf.pin_bit_mask = (1ULL<<RGB_1) | (1ULL<<RGB_2) | (1ULL<<RGB_3);
	gpio_conf.mode = GPIO_MODE_OUTPUT;
    gpio_config(&gpio_conf);    
}

static void btnClicked(uint32_t io_num, TypeClick type)
{
    if (type == Simple && !lcdIsRunning()){
        startLcd();
    }
    switch (io_num){
        case BTN_1:
        if (type == Simple){
            setNextPage((LcdPageIndex)(getCurrentPage() - 1));
        } else if (type == Double){
            setNextPage((LcdPageIndex)(getCurrentPage() - 2));
        }
        break;

        case BTN_2:
        if (type == Simple){
            setNextPage((LcdPageIndex)(getCurrentPage() + 1));
        } else if (type == Double){
            setNextPage((LcdPageIndex)(getCurrentPage() + 2));
        }
        break;
    }
    if (type == VeryLong){
        stopLcd();
    }
}

void	app_main() {
    WifiConfig_t	dataWifi = {
        .ssid = "Honor Raphael",
        .password = "clemon69"
    };
    MqttConfig_t	dataMqtt = {
        .url = "mqtt://iot.eclipse.org"
    };

    nvs_flash_init();
    setupLeds();
    setButtonCallback(&btnClicked);
    startButtonClient();

	startWifiClient(&dataWifi);
	startMqttClient(&dataMqtt);
    startLcd();
	//startSensorClient();
}