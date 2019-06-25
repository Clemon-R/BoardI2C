#include "HumidityTemperatureSensor.h"
#include "../I2CByteManager.h"

static const char TAG[] = "\033[1;94mHumidityTemperatureSensor\033[0m";

static esp_err_t	initTemperatureCalibration(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int ret;
	uint8_t	tmp = 0;
	uint8_t *params;

	if (args == NULL)
		return ESP_FAIL;
	ESP_LOGI(TAG, "Asking Temperature Calibration values...");
	params = (uint8_t *)&args->T0_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_BURST | 0x3c, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYTEMP, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	params = (uint8_t *)&args->T1_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_BURST | 0x3e, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYTEMP, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, 0x32, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYTEMP, (uint8_t *)&(args->T0_deg));

	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, 0x33, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYTEMP, (uint8_t *)&(args->T1_deg));
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, 0x35, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYTEMP, &tmp);

	args->T0_deg /= 8;
	args->T1_deg /= 8;
	args->T0_deg += tmp & 0x03;
	args->T1_deg += tmp & 0x0c;
	return ESP_OK;
}

static esp_err_t initHumidityCalibration(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int ret;
	uint8_t *params;

	if (args == NULL)
		return ESP_FAIL;
	ESP_LOGI(TAG, "Asking Humidity Calibration values...");
	params = (uint8_t *)&args->H0_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_BURST | 0x36, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYTEMP, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	params = (uint8_t *)&args->H1_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_BURST | 0x3A, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYTEMP, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, 0x30, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYTEMP, &args->H0_rh);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, 0x31, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYTEMP, &args->H1_rh);

	args->H1_rh /= 2;
	args->H0_rh /= 2;
	return ret;
}

esp_err_t	setupHumidityTempSensor(i2c_port_t port, humidity_temp_sensor_t *data)
{
	esp_err_t	ret;

	if (!data)
		return ESP_FAIL;
	data->T0_deg = 0;
	data->T1_deg = 0;
	ret = initTemperatureCalibration(port, data);
	ret = initHumidityCalibration(port, data);
	return ESP_OK;
}

esp_err_t	initHumidityTempSensor(i2c_port_t port)
{
	uint8_t data;
	int ret;

	ESP_LOGI(TAG, "Asking WHOIAM for sensor Humidity/Temperature...");
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_WHOIAM, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYTEMP, &data);
	if (ret != ESP_OK || data != HUMIDITYTEMP_WHOIAM_ANS)
		return ESP_FAIL;
		
	data = HUMIDITYTEMP_ODR;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_CTRL_REG, &data);
	if (ret != ESP_OK)
		return ESP_FAIL;

	data = HUMIDITYTEMP_AVG;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_AV_CONF, &data);
	return ret;
}

float	getTemperature(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int16_t	T_out;
	uint8_t *params;
	int	ret;

	if (args == NULL)
		return -1;
	params = (uint8_t *)&T_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_BURST | HUMIDITYTEMP_TEMPL, NULL);
	if (ret != ESP_OK)
		return -1;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYTEMP, params, 2);
	if (ret != ESP_OK)
		return -1;
	return ((args->T1_deg - args->T0_deg) * (T_out - args->T0_out)) / (float)(args->T1_out - args->T0_out) + args->T0_deg;
}


float	getHumidity(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int16_t	H_out;
	uint8_t *params;
	int	ret;

	if (args == NULL)
		return -1;
	params = (uint8_t *)&H_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYTEMP, HUMIDITYTEMP_BURST | HUMIDITYTEMP_HUMIDITYL, NULL);
	if (ret != ESP_OK)
		return -1;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYTEMP, params, 2);
	if (ret != ESP_OK)
		return -1;
	return ((args->H1_rh - args->H0_rh) * (H_out - args->H0_out)) / (float)(args->H1_out - args->H0_out) + args->H0_rh;
}