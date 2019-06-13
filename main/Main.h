#ifndef APPMAIN_H_
#define APPMAIN_H_

#include "sdkconfig.h"
#include "esp_log.h"
#include "sensors/HumidityTemperatureSensor.h"
#include "sensors/PressureTemperatureSensor.h"
#include "sensors/ColorSensor.h"

#define RGB_1 16
#define RGB_2 17
#define RGB_3 2

typedef struct main_data_s
{
	humidity_temp_sensor_t	humidityData;
	pressure_temp_sensor_t	pressureData;
}				main_data_t;

#endif // !APPMAIN_H_