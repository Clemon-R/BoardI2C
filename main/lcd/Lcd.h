#ifndef	LCD_H_
#define	LCD_H_

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_system.h"

//LCD
#define PIN_NUM_MISO (gpio_num_t)25
#define PIN_NUM_MOSI (gpio_num_t)23
#define PIN_NUM_CLK  (gpio_num_t)19
#define PIN_NUM_CS   (gpio_num_t)22

#define PIN_NUM_DC   (gpio_num_t)21
#define PIN_NUM_RST  (gpio_num_t)18
#define PIN_NUM_BCKL (gpio_num_t)5

#define BUFF_SIZE	1024

typedef enum	LcdPageIndex_e
{
	SENSORS = 0,
	LOGS,
	SETTINGS
}				LcdPageIndex;

void	changePage(uint8_t index);

esp_err_t	startLcd();
esp_err_t	stopLcd();

#endif