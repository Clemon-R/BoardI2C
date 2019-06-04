/**
 * Based on Sensor LPS22HB
 * 
 * Author: Raphaël-G
 **/

#ifndef PRESSURETEMP_H_
#define PRESSURETEMP_H_

#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define I2C_SLAVE_PRESSURETEMP	0x5C

#define PRESSURETEMP_CTRL_REG1	0x10
#define PRESSURETEMP_WHOIAM	0x0F
#define PRESSURETEMP_WHOIAM_ANS	0xB1

#define PRESSURETEMP_ODR2 0
#define PRESSURETEMP_ODR1 1
#define PRESSURETEMP_ODR0 0
#define PRESSURETEMP_ODR ((PRESSURETEMP_ODR2 << 6) | (PRESSURETEMP_ODR1 << 5) | (PRESSURETEMP_ODR0 << 4))

typedef struct pressure_temp_sensor_s
{
	int16_t	offset;
}				pressure_temp_sensor_t;

esp_err_t	initPressureTempSensor(i2c_port_t port);
pressure_temp_sensor_t	setupPressureTempSensor(i2c_port_t port);




#endif // !PRESSURETEMP_H_