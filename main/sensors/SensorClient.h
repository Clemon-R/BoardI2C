#ifndef SENSORCLIENT_H_
#define SENSORCLIENT_H_

#include "sdkconfig.h"
#include "esp_log.h"

#include "HumidityTemperatureSensor.h"
#include "PressureTemperatureSensor.h"
#include "ColorSensor.h"

//Sensor
#define	I2C_MASTER_NUM	(i2c_port_t)0
#define I2C_MASTER_SDA_IO	(gpio_num_t)4
#define I2C_MASTER_SCL_IO	(gpio_num_t)0
#define I2C_MASTER_FREQ_HZ	10000

typedef struct SensorData_s
{
	humidity_temp_sensor_t	humidityData;
	pressure_temp_sensor_t	pressureData;
}				SensorData_t;

esp_err_t	startSensorClient();
esp_err_t	stopSensorClient();

#endif // !SENSORCLIENT_H_