#ifndef APPMAIN_H_
#define APPMAIN_H_

#include "sdkconfig.h"
#include "esp_log.h"
#include "sensors/HumidityTemperatureSensor.h"
#include "sensors/PressureTemperatureSensor.h"
#include "sensors/ColorSensor.h"

#define RGB_1_RED 32
#define RGB_1_GREEN 15
#define RGB_2_RED 33
#define RGB_2_GREEN 12
#define RGB_3_RED 2
#define RGB_3_GREEN 13
#define RGB_3_BLUE 14
#define RGB_MASK    ((1ULL<<RGB_1_RED) | (1ULL<<RGB_1_GREEN) | (1ULL<<RGB_2_RED) | (1ULL<<RGB_2_GREEN) | (1ULL<<RGB_3_RED) | (1ULL<<RGB_3_GREEN) | (1ULL<<RGB_3_BLUE))

#define	BTN_LEFT (gpio_num_t)35
#define	BTN_RIGHT (gpio_num_t)27
#define	BTN_TOP (gpio_num_t)34
#define	BTN_BOTTOM (gpio_num_t)39
#define	BTN_CLICK (gpio_num_t)26
#define BTN_MASK    ((1ULL<<BTN_LEFT) | (1ULL<<BTN_RIGHT) | (1ULL<<BTN_TOP) | (1ULL<<BTN_BOTTOM) | (1ULL<<BTN_CLICK))

typedef struct main_data_s {
    humidity_temp_sensor_t	humidityData;
    pressure_temp_sensor_t	pressureData;
}				main_data_t;

const char  *getCurrentVersion();

#endif // !APPMAIN_H_