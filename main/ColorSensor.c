#include "ColorSensor.h"
#include "I2CByteManager.h"

static char	TAG[] = "ColorSensor";

esp_err_t	initColorSensor(i2c_port_t port)
{
	uint8_t	data = 0;
	int	ret;
	
	ESP_LOGI(TAG, "Asking SYSTEM_CONTROL for sensor Color...");
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_SYSTEM_CONTROL, NULL);
	if (ret != ESP_OK)
		return ret;
	ret = readByte(port, I2C_SLAVE_COLOR, &data);
	if (ret != ESP_OK || (data & 0x3f) != 0xb)
		return ESP_FAIL;
	return ret;
}

void	setupColorSensor(i2c_port_t port)
{
	int	ret;
	uint8_t	data;
	
	data = 0;
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_MODE_CONTROL1, &data);
	ESP_ERROR_CHECK(ret);

	data = 0x90;
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_MODE_CONTROL2, &data);
	ESP_ERROR_CHECK(ret);
}

color_rgb_t	getColorRGB(i2c_port_t port)
{
	int	ret;
	color_rgb_t	result = {0,0,0};

	ESP_LOGI(TAG, "Asking RGB...");
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_REDL, NULL);
	ESP_ERROR_CHECK(ret);
	ret = readBytes(port, I2C_SLAVE_COLOR, (uint8_t *)&result, 6);
	ESP_ERROR_CHECK(ret);
	return result;
}