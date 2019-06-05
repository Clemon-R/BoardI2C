#include "PressureTemperatureSensor.h"
#include "I2CByteManager.h"

static const char TAG[] = "PressureTemperatureSensor";

esp_err_t	initPressureTempSensor(i2c_port_t port)
{
	uint8_t data;
	int ret;

	ESP_LOGI(TAG, "Asking WHOIAM for sensor Pressure/Temperature...");
	ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEMP_WHOIAM, NULL);
	if (ret != ESP_OK)
		return ESP_FAIL;
	ret = readByte(port, I2C_SLAVE_PRESSURETEMP, &data);
	if (ret != ESP_OK || data != PRESSURETEMP_WHOIAM_ANS)
		return ESP_FAIL;
	return ret;
}


pressure_temp_sensor_t	setupPressureTempSensor(i2c_port_t port)
{
	uint8_t data;
	int ret;
	pressure_temp_sensor_t	result;

	data = PRESSURETEMP_ODR;
	ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEMP_CTRL_REG1, &data);
	ESP_ERROR_CHECK(ret);

	ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEP_RPDSL, (uint8_t *)&result.offset);
	ESP_ERROR_CHECK(ret);

	return result;
}


int32_t	getPressure(i2c_port_t port)
{
	int ret;
	int32_t	result = 0;

	ESP_LOGI(TAG, "Asking Pressure...");
	ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEP_PRESS_OUTXL, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readBytes(port, I2C_SLAVE_PRESSURETEMP, (uint8_t *)&result, 3);
	ESP_ERROR_CHECK(ret);

	return result / 4096;
}