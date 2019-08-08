#ifndef SENSORCLIENT_H_
#define SENSORCLIENT_H_

#include "../../demo_conf.h"

#include "sdkconfig.h"
#include "esp_log.h"

#include "HumidityTemperatureSensor.h"
#include "PressureTemperatureSensor.h"
#include "ColorSensor.h"


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