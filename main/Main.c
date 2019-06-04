
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/task.h"

#include "HumidityTemperatureSensor.h"

//Sensor
#define	I2C_MASTER_NUM	(i2c_port_t)0
#define I2C_MASTER_SDA_IO	(gpio_num_t)4
#define I2C_MASTER_SCL_IO	(gpio_num_t)0
#define I2C_MASTER_FREQ_HZ	10000

static char	TAG[] = "Main";

esp_err_t nordicI2CInit()
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

static humidity_temp_sensor_t setupAllSensors()
{
	ESP_ERROR_CHECK(initHumidityTempSensor(I2C_MASTER_NUM));
	return setupHumidityTempSensor(I2C_MASTER_NUM);
}

void	app_main(){
	humidity_temp_sensor_t	data;
	
	ESP_ERROR_CHECK(nordicI2CInit());

	data = setupAllSensors();
	while (1){
		ESP_LOGI(TAG, "Result Temperature: %d", getTemperature(I2C_MASTER_NUM, &data));
		ESP_LOGI(TAG, "Result Humidity: %d", getHumidity(I2C_MASTER_NUM, &data));
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}