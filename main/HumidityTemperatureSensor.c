#include "HumidityTemperatureSensor.h"
#include "I2CByteManager.h"

static char *TAG = "HumidityTemperatureSensor";

static esp_err_t	initTemperatureCalibration(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int ret;
	uint8_t	tmp = 0;
	uint8_t *params;

	if (args == NULL)
		return ESP_FAIL;
	ESP_LOGI(TAG, "Asking Temperature Calibration values...");
	params = (uint8_t *)&args->T0_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_BURST | 0x3c, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYPRESSURE, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	params = (uint8_t *)&args->T1_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_BURST | 0x3e, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYPRESSURE, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, 0x32, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYPRESSURE, (uint8_t *)&(args->T0_deg));

	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, 0x33, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYPRESSURE, (uint8_t *)&(args->T1_deg));
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, 0x35, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYPRESSURE, &tmp);

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
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_BURST | 0x36, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYPRESSURE, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	params = (uint8_t *)&args->H1_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_BURST | 0x3A, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readBytes(port, I2C_SLAVE_HUMIDITYPRESSURE, params, 2);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, 0x30, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYPRESSURE, &args->H0_rh);
	if (ret != ESP_OK)
		return ESP_FAIL;

	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, 0x31, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYPRESSURE, &args->H1_rh);

	args->H1_rh /= 2;
	args->H0_rh /= 2;
	return ret;
}

humidity_temp_sensor_t	setupHumidityTempSensor(i2c_port_t port)
{
	humidity_temp_sensor_t	data;

	data.T0_deg = 0;
	data.T1_deg = 0;
	ESP_ERROR_CHECK(initTemperatureCalibration(port, &data));
	ESP_ERROR_CHECK(initHumidityCalibration(port, &data));
	return data;
}

esp_err_t	initHumidityTempSensor(i2c_port_t port)
{
	uint8_t data;
	int ret;

	ESP_LOGI(TAG, "Asking WHOIAM to captor Temperature/Pressure...");
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_WHOIAM, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_HUMIDITYPRESSURE, &data);
	if (ret != ESP_OK || data != HUMIDITYPRESSURE_WHOIAM_ANS)
		return ESP_FAIL;
		
	data = HUMIDITYPRESSURE_ODR;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_CTRL_REG, &data);
	if (ret != ESP_OK)
		return ESP_FAIL;

	data = HUMIDITYPRESSURE_AVG;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_AV_CONF, &data);
	return ret;
}

int16_t	getTemperature(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int16_t	T_out;
	uint8_t *params;
	int	ret;

	ESP_ERROR_CHECK(args == NULL);
	ESP_LOGI(TAG, "Asking Temperature...");
	params = (uint8_t *)&T_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_BURST | HUMIDITYPRESSURE_TEMPL, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readBytes(port, I2C_SLAVE_HUMIDITYPRESSURE, params, 2);
	ESP_ERROR_CHECK(ret);
	return ((args->T1_deg - args->T0_deg) * (T_out - args->T0_out)) / (args->T1_out - args->T0_out) + args->T0_deg;
}


int16_t	getHumidity(i2c_port_t port, humidity_temp_sensor_t *args)
{
	int16_t	H_out;
	uint8_t *params;
	int	ret;

	ESP_ERROR_CHECK(args == NULL);
	ESP_LOGI(TAG, "Asking Humidity...");
	params = (uint8_t *)&H_out;
	ret = writeByte(port, I2C_SLAVE_HUMIDITYPRESSURE, HUMIDITYPRESSURE_BURST | HUMIDITYPRESSURE_HUMIDITYL, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readBytes(port, I2C_SLAVE_HUMIDITYPRESSURE, params, 2);
	ESP_ERROR_CHECK(ret);
	return ((args->H1_rh - args->H0_rh) * (H_out - args->H0_out)) / (args->H1_out - args->H0_out) + args->H0_rh;
}