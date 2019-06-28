#include "ColorSensor.h"
#include "../I2CByteManager.h"

static const char	TAG[] = "\033[1;94mColorSensor\033[0m";

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

esp_err_t	setupColorSensor(i2c_port_t port)
{
	int	ret;
	uint8_t	data;
	
	data = 0;
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_MODE_CONTROL1, &data);
	if (ret != ESP_OK)
		return ret;

	data = 0x92;
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_MODE_CONTROL2, &data);


	data = 0x02;
	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_MODE_CONTROL3, &data);
	return ret;
}

color_rgb_t	getColorRGB(i2c_port_t port)
{
	int	ret;
	color_rgb_t	result;

	ret = writeByte(port, I2C_SLAVE_COLOR, COLOR_REDL, NULL);
	if (ret != ESP_OK)
		return (color_rgb_t){false, 0, 0, 0};
	ret = readBytes(port, I2C_SLAVE_COLOR, (uint8_t *)&result, 6);
	if (ret != ESP_OK)
		return (color_rgb_t){false, 0, 0, 0};
	result.available = true;
	return result;
}