#ifndef LCDSTATE_H_
#define LCDSTATE_H_

#include "Lcd.h"

typedef struct LcdState_s
{
	lv_obj_t	*lblTemperature;
	lv_obj_t	*lblHumidity;
	lv_obj_t	*lblPressure;
	lv_obj_t	*lblColor;

	lv_obj_t	*ledWifi;
	lv_obj_t	*ledMqtt;
	lv_obj_t	*ledSensors;
}				LcdState_t;

void	createStatesView(void *tab);
LcdState_t	*getStateData();

#endif