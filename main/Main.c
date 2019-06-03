
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"
#include "freertos/task.h"

#include "I2CByteManager.h"

//Sensor
#define	I2C_SLAVE_HUMIDITYPRESSURE	0x5F
#define	I2C_SLAVE_ADD2	0x69
#define	I2C_MASTER_NUM	(i2c_port_t)0
#define I2C_MASTER_SDA_IO	(gpio_num_t)4
#define I2C_MASTER_SCL_IO	(gpio_num_t)0
#define I2C_MASTER_FREQ_HZ	10000

#define HUMIDITYPRESSURE_WHOIAM 0x0F
#define HUMIDITYPRESSURE_WHOIAM_ANS 0xBC
#define HUMIDITYPRESSURE_HUMIDITYL 0x28
#define HUMIDITYPRESSURE_HUMIDITYH 0x29
#define HUMIDITYPRESSURE_TEMPL 0x2A
#define HUMIDITYPRESSURE_TEMPH 0x2B
#define HUMIDITYPRESSURE_CTRL_REG 0x20

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

static void setupAllSensors()
{
	uint8_t data;
	int ret;

	data = 0b00000011;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_CTRL_REG, &data);
	ESP_ERROR_CHECK(ret);
}

static int16_t getTemperature(){
	int ret;
	int16_t	T_OUT = 0;
	int16_t	T0_OUT = 0;
	int16_t	T1_OUT = 0;
	uint16_t	T0_DEG = 0;
	uint16_t	T1_DEG = 0;
	uint8_t	tmp = 0;
	uint8_t *params;

	ESP_LOGI(TAG, "Asking Temperature...");
	params = (uint8_t *)&T_OUT;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_TEMPL, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[0]);
	ESP_ERROR_CHECK(ret);
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_TEMPH, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[1]);

	params = (uint8_t *)&T0_OUT;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x3c, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[0]);
	ESP_ERROR_CHECK(ret);
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x3d, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[1]);
	ESP_ERROR_CHECK(ret);

	params = (uint8_t *)&T1_OUT;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x3e, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[0]);
	ESP_ERROR_CHECK(ret);
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x3f, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[1]);

	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x32, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, (uint8_t *)(&T0_DEG));

	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x33, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, (uint8_t *)(&T1_DEG));
	ESP_ERROR_CHECK(ret);

	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x35, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &tmp);
	ESP_ERROR_CHECK(ret);

	T0_DEG /= 8;
	T1_DEG /= 8;
	T0_DEG += tmp & 0x03;
	T1_DEG += tmp & 0x0c;
	return ((T1_DEG - T0_DEG) * (T_OUT - T0_OUT)) / (T1_OUT - T0_OUT) + T0_DEG;
}

static int16_t getHumidity(){
	int ret;
	int16_t    H_OUT = 0;
	int16_t    H0_OUT = 0;
	int16_t    H1_OUT = 0;
	uint8_t    H0_RH = 0;
	uint8_t    H1_RH = 0;
	uint8_t *params;

	ESP_LOGI(TAG, "Asking Humidity...");
	params = (uint8_t *)&H_OUT;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_HUMIDITYL, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[0]);
	ESP_ERROR_CHECK(ret);
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_HUMIDITYH, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[1]);
	ESP_ERROR_CHECK(ret);

	params = (uint8_t *)&H0_OUT;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x36, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[0]);
	ESP_ERROR_CHECK(ret);
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x37, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[1]);
	ESP_ERROR_CHECK(ret);

	params = (uint8_t *)&H1_OUT;
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x3A, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[0]);
	ESP_ERROR_CHECK(ret);
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x3B, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &params[1]);
	ESP_ERROR_CHECK(ret);

	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x30, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &H0_RH);
	ESP_ERROR_CHECK(ret);

	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, 0x31, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &H1_RH);
	ESP_ERROR_CHECK(ret);
	H1_RH /= 2;
	H0_RH /= 2;
	return ((H1_RH - H0_RH) * (H_OUT - H0_OUT)) / (H1_OUT - H0_OUT) + H0_RH;
}

void	app_main(){
	int ret;
	uint8_t data = 0;
	
	ESP_ERROR_CHECK(nordicI2CInit());

	ESP_LOGI(TAG, "Asking WHOIAM to captor Temperature/Pressure...");
	ret = writeByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_WHOIAM, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readByte(I2C_MASTER_NUM, I2C_SLAVE_HUMIDITYPRESSURE, &data);
	ESP_ERROR_CHECK(ret);
	ESP_ERROR_CHECK(data != HUMIDITYPRESSURE_WHOIAM_ANS);
	ESP_LOGI(TAG, "Result: 0x%02X", data);

	setupAllSensors();
	while (1){
		ESP_LOGI(TAG, "Result Temperature: %d", getTemperature());
		ESP_LOGI(TAG, "Result Humidity: %d", getHumidity());
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}