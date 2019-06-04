/**
 * Based on Sensor HTS221
 * 
 * Author: Raphaël-G
 **/

#ifndef HUMIDITYTEMP_H_
#define HUMIDITYTEMP_H_

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define	I2C_SLAVE_HUMIDITYPRESSURE	0x5F
#define HUMIDITYPRESSURE_BURST 0x80

#define HUMIDITYPRESSURE_WHOIAM 0x0F
#define HUMIDITYPRESSURE_AV_CONF 0x10
#define HUMIDITYPRESSURE_WHOIAM_ANS 0xBC
#define HUMIDITYPRESSURE_HUMIDITYL 0x28
#define HUMIDITYPRESSURE_HUMIDITYH 0x29
#define HUMIDITYPRESSURE_TEMPL 0x2A
#define HUMIDITYPRESSURE_TEMPH 0x2B
#define HUMIDITYPRESSURE_CTRL_REG 0x20

#define HUMIDITYPRESSURE_AVGT 2
#define HUMIDITYPRESSURE_AVGH 2
#define HUMIDITYPRESSURE_AVG ((HUMIDITYPRESSURE_AVGT << 3) & HUMIDITYPRESSURE_AVGH)

#define HUMIDITYPRESSURE_ODR1 2
#define HUMIDITYPRESSURE_ODR0 2
#define HUMIDITYPRESSURE_ODR ((HUMIDITYPRESSURE_ODR1 << 1) & HUMIDITYPRESSURE_ODR0)

typedef struct	humidity_temp_sensor_s
{
	//Values of calibration temperature
	int16_t	T0_out;
	int16_t	T1_out;
	uint16_t	T0_deg;
	uint16_t	T1_deg;	

	//Values of calibration humidity
	int16_t	H0_out;
	int16_t	H1_out;
	uint8_t	H0_rh;
	uint8_t	H1_rh;
}				humidity_temp_sensor_t;

esp_err_t	initHumidityTempSensor(i2c_port_t port);
humidity_temp_sensor_t	setupHumidityTempSensor(i2c_port_t port);

int16_t	getTemperature(i2c_port_t port, humidity_temp_sensor_t *data);
int16_t	getHumidity(i2c_port_t port, humidity_temp_sensor_t *data);

#endif // !HUMIDITYTEMP_H_