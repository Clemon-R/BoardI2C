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

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64
#define ADC_CHANNEL     ADC_CHANNEL_0
#define ADC_ATTEN       ADC_ATTEN_DB_0   

typedef struct SensorData_s {
    humidity_temp_sensor_t	humidityData;
    pressure_temp_sensor_t	pressureData;
}				SensorData_t;

typedef struct  SensorValues_s {
    float     temperature;
    float     humidity;
    float     pressure;
    color_rgb_t color;
    char        initiated:1;
}               SensorValues_t;

esp_err_t	startSensorClient();
esp_err_t	stopSensorClient();

char	isSensorRunning();
char	isSensorWorking();

void	setRefreshDelai(TickType_t value);
TickType_t  getRefreshDelai();
SensorData_t	*getSensorConfig();

SensorValues_t  getSensorValues();
void    getBatteryVoltage();

#endif // !SENSORCLIENT_H_