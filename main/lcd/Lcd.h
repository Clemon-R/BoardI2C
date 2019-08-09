#ifndef	LCD_H_
#define	LCD_H_

#include "../../demo_conf.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "../../lvgl/lvgl.h"

#include "Sensors.h"
#include "Log.h"
#include "State.h"


typedef enum	LcdPageIndex_e {
    SENSORS = 0,
    LOGS,
    STATES
}				LcdPageIndex;

typedef struct	ValueContainer_s {
    lv_obj_t	*value;
    lv_obj_t	*decimale;
}				ValueContainer_t;

void	nextPage();
void	previousPage();
int32_t getCurrentPage();

esp_err_t	startLcd();
esp_err_t	stopLcd();

char	lcdIsRunning();
char	lcdIsWorking();

SemaphoreHandle_t	lcdGetSemaphore();
void    updateProgress(uint8_t value, const char show);

#endif