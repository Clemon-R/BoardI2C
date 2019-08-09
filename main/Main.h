#ifndef APPMAIN_H_
#define APPMAIN_H_

#include "../demo_conf.h"

#include "sdkconfig.h"
#include "esp_log.h"
#include "sensors/HumidityTemperatureSensor.h"
#include "sensors/PressureTemperatureSensor.h"
#include "sensors/ColorSensor.h"


typedef struct main_data_s {
    humidity_temp_sensor_t	humidityData;
    pressure_temp_sensor_t	pressureData;
}				main_data_t;

#endif // !APPMAIN_H_