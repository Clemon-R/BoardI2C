#ifndef LCDSENSORS_H_
#define LCDSENSORS_H_

#include "Lcd.h"

struct	ValueContainer_s;
typedef	struct	ValueContainer_s	ValueContainer_t;

typedef enum	SensorsPage_e
{
	ALL = 0,
	TEMPERATURE,
	HUMIDITY,
	PRESSURE
}				SensorsPage_t;

typedef struct	LcdSensors_s
{
	lv_obj_t	*red;
	lv_obj_t	*green;
	lv_obj_t	*blue;
	lv_obj_t	*rgb;

	ValueContainer_t	*temperature;
	ValueContainer_t	*humidity;
	ValueContainer_t	*pressure;

	lv_obj_t *meterTemperature;
	lv_obj_t *meterHumidity;

	lv_obj_t *meterLblTemperature;
	lv_obj_t *meterLblHumidity;
	lv_obj_t *meterLblPressure;

	lv_obj_t *chartTemperature;
	lv_chart_series_t *chartSerTemperature;
	lv_obj_t *chartHumidity;
	lv_chart_series_t *chartSerHumidity;
	lv_obj_t *chartPressure;
	lv_chart_series_t *chartSerPressure;

	lv_obj_t	*navMenu;
	lv_obj_t	*nav;
	lv_obj_t	*oldBtn;  
}				LcdSensors_t;

void	createSensorsView(void *tab);
LcdSensors_t	*getSensorsData();

SensorsPage_t getSensorsCurrentPage();
void    previousSensorsPage();
void    nextSensorsPage();

void    showContainer(uint32_t code, char show);

#endif