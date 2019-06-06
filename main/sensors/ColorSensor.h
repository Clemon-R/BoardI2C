/**
 * Based on Sensor BH1745NUC
 * 
 * Author: Raphaël-G
 **/

#ifndef COLORSENSOR_H_
#define COLORSENSOR_H_

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define I2C_SLAVE_COLOR	0x38

#define	COLOR_SYSTEM_CONTROL	0x40
#define COLOR_SYSTEM_CONTROL_ANS	0x0B
#define	COLOR_MODE_CONTROL1	0x41
#define	COLOR_MODE_CONTROL2	0x42
#define COLOR_REDL	0x50
#define COLOR_REDH	0x51
#define COLOR_GREENL	0x52
#define COLOR_GREENH	0x53
#define COLOR_BLUEL	0x54
#define COLOR_BLUER	0x55

typedef struct	color_rgb_s
{
	uint16_t	r;
	uint16_t	g;
	uint16_t	b;
}				color_rgb_t;

esp_err_t	initColorSensor(i2c_port_t port);
void	setupColorSensor(i2c_port_t port);

color_rgb_t	getColorRGB(i2c_port_t port);

#endif // !COLORSENSOR_H_