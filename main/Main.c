
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "Main.h"
#include "wifi/WifiClient.h"
#include "mqtt/MqttClient.h"

//Sensor
#define	I2C_MASTER_NUM	(i2c_port_t)0
#define I2C_MASTER_SDA_IO	(gpio_num_t)4
#define I2C_MASTER_SCL_IO	(gpio_num_t)0
#define I2C_MASTER_FREQ_HZ	10000

static char	TAG[] = "Main";

const static int WIFI_CONNECTED = BIT0;
const static int MQTT_CONNECTED = BIT1;

static esp_err_t nordicI2CInit()
{
	i2c_port_t i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;

	ESP_LOGI(TAG, "Initiating I2C for master...");
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	conf.scl_io_num = I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

	i2c_param_config(i2c_master_port, &conf);
	if (i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0) != ESP_OK){
		ESP_LOGE(TAG, "Failed to init I2C for master");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "Successfull initiated I2C for master");
	return ESP_OK;
}

static main_data_t setupAllSensors()
{
	main_data_t	result;

	ESP_ERROR_CHECK(initHumidityTempSensor(I2C_MASTER_NUM));
	ESP_ERROR_CHECK(initPressureTempSensor(I2C_MASTER_NUM));
	ESP_ERROR_CHECK(initColorSensor(I2C_MASTER_NUM));
	result.pressureData = setupPressureTempSensor(I2C_MASTER_NUM);
	result.humidityData = setupHumidityTempSensor(I2C_MASTER_NUM);
	setupColorSensor(I2C_MASTER_NUM);
	return result;
}

void	app_main(){
	main_data_t	data;
	color_rgb_t	tmp;
	EventGroupHandle_t	eventGroup;
	
    nvs_flash_init();
	ESP_ERROR_CHECK(nordicI2CInit());
	data = setupAllSensors();

	eventGroup = xEventGroupCreate();
	ESP_ERROR_CHECK(eventGroup == NULL);
	launchWifi(eventGroup, "Honor Raphael", "clemon69");
	launchMqtt(eventGroup, "");
	while (1){
		if ((xEventGroupWaitBits(eventGroup, WIFI_CONNECTED, false, true, portMAX_DELAY) & WIFI_CONNECTED) != WIFI_CONNECTED)
			continue;
		if ((xEventGroupWaitBits(eventGroup, MQTT_CONNECTED, false, true, portMAX_DELAY) & MQTT_CONNECTED) != MQTT_CONNECTED)
			continue;
		ESP_LOGI(TAG, "Result Temperature: %d°C", getTemperature(I2C_MASTER_NUM, &data.humidityData));
		ESP_LOGI(TAG, "Result Humidity: %d%c", getHumidity(I2C_MASTER_NUM, &data.humidityData), '%');
		ESP_LOGI(TAG, "Result Pressure: %dhPa", getPressure(I2C_MASTER_NUM));

		tmp = getColorRGB(I2C_MASTER_NUM);
		ESP_LOGI(TAG, "Result RGB R: 0x%2x, G: 0x%2x, B: 0x%2x", tmp.r, tmp.g, tmp.r);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}