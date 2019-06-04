#ifndef APPMAIN_H_
#define APPMAIN_H_

#include "sdkconfig.h"
#include "esp_log.h"
#include "HumidityTemperatureSensor.h"
#include "PressureTemperatureSensor.h"
#include "ColorSensor.h"

typedef struct main_data_s
{
	humidity_temp_sensor_t	humidityData;
	pressure_temp_sensor_t	pressureData;
}				main_data_t;

#endif // !APPMAIN_H_