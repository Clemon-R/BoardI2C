#include "PressureTemperatureSensor.h"
#include "I2CByteManager.h"

static char TAG[] = "PressureTemperatureSensor";

esp_err_t	initPressureTempSensor(i2c_port_t port)
{
	uint8_t data;
	int ret;

	ESP_LOGI(TAG, "Asking WHOIAM to captor Pressure/Temperature...");
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

	ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, 0x18, (uint8_t *)&result.offset);
	ESP_ERROR_CHECK(ret);

	return result;
}