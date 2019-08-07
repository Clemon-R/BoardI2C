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
#define NO_OF_SAMPLES   128
#define ADC_CHANNEL     ADC_CHANNEL_0
#define ADC_ATTEN       ADC_ATTEN_DB_0 

#define BATTERIE_MAX_VOLTAGE 4200 //In mV
#define DOWNGRADE_VOLTAGE 4500 //In mV
#define NBR_TENSION_SAMPLE 13
#define ADC_VOLTAGE 1100 //In mV /*WARNING*\ Do not change

#define PERCENTAGE_REAL (BATTERIE_MAX_VOLTAGE / (double)DOWNGRADE_VOLTAGE)
#define MAX_ADC_VOLTAGE (ADC_VOLTAGE * PERCENTAGE_REAL)
#define REAL_VOLTAGE(fakeV) (BATTERIE_MAX_VOLTAGE / (double)MAX_ADC_VOLTAGE * fakeV)

typedef struct SensorData_s {
    humidity_temp_sensor_t	humidityData;
    pressure_temp_sensor_t	pressureData;
}				SensorData_t;

typedef struct  SensorValues_s {
    float     temperature;
    float     humidity;
    float     pressure;
    int8_t    battery;  
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

#endif // !SENSORCLIENT_H_