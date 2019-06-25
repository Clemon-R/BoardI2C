#include "Lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "freertos/event_groups.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"

//#include "../wifi/WifiClient.h"
#include "../mqtt/MqttClient.h"
#include "../sensors/SensorClient.h"

#include "../../drv/disp_spi.h"
#include "../../drv/ili9341.h"
#include "../../lvgl/lvgl.h"

#include <math.h>

static const char	*TAG = "\033[1;93mLcd\033[0m";
static TaskHandle_t	lcdTask = NULL;

static char	_running = false;
static char	_working = false;
static SemaphoreHandle_t	_semaphore = NULL;

static lv_obj_t	*_logs = NULL;

static int8_t	_currentIndex = 0;
static int8_t	_nextIndex = -1;

static lv_obj_t	*_tv = NULL;

static void IRAM_ATTR lv_tick_task(void)
{
	lv_tick_inc(portTICK_RATE_MS);
}

static int	customVPrintF(const char *str, va_list arg)
{
	static char	**logs = NULL;
	static char	**next = NULL;
	static uint8_t	index = 0;
	char	buffer[1024];

	if (!logs){
		logs = (char **)malloc(sizeof(char *) * 15);
		memset(logs, NULL, sizeof(char *) * 15);
		next = logs;
	}
	if (_logs){
		vsprintf(buffer, str, arg);
		if (!*next){
			*next = strdup(buffer);
			if (index < 13){
				index++;
				next = logs + index;
			}
		} else {
			for (int i = 0;i < index;i++){
				if (i == 0){
					free(logs[i]);
				}
				logs[i] = logs[i + 1];
			}
			*next = strdup(buffer);
		}
		if (xSemaphoreTake(_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE){
			lv_ta_set_text(_logs, "");
			for (int i = 0;logs && logs[i];i++){
				lv_ta_add_text(_logs, logs[i]);
			}
			xSemaphoreGive(_semaphore);
		}
	}
	return vprintf(str, arg);
}

static esp_err_t	initLcd()
{
	esp_err_t	ret;

	ESP_LOGI(TAG, "Initiating the LCD...");
	lv_init();
	ret = disp_spi_init();
	if (ret != ESP_OK)
		return ret;
	ili9341_init();

	lv_disp_drv_t disp;
	lv_disp_drv_init(&disp);
	disp.disp_flush = ili9341_flush;
    disp.disp_fill = ili9341_fill;
	ret = lv_disp_drv_register(&disp) != NULL ? ESP_OK : ESP_FAIL;
	if (ret != ESP_OK)
		return ret;
	esp_log_set_vprintf(&customVPrintF);
	return esp_register_freertos_tick_hook(lv_tick_task);
}

static esp_err_t	deinitLcd()
{
	ESP_LOGI(TAG, "Deinitiating the LCD...");
	esp_log_set_vprintf(&vprintf);
	_logs = NULL;
	ili9341_deinit();
	lv_disp_set_active(NULL);
	return disp_spi_deinit();
}

static void	createLogsView(void *tab)
{
	lv_obj_t	*parent = (lv_obj_t *)tab;
	lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
	lv_page_set_scrl_fit(parent, false, true);
	lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
	lv_style_t	*styleTab = lv_obj_get_style(parent);
	styleTab->body.padding.hor = 0;

	static lv_style_t	style;
	lv_style_copy(&style, (lv_theme_get_current()->ta.oneline));
	style.text.font = &lv_font_dejavu_10;

	lv_obj_t	*textarea = lv_ta_create(parent, NULL);
	lv_obj_set_style(textarea, &style);
	lv_obj_set_size(textarea, lv_obj_get_width(parent), lv_obj_get_height(parent));
	lv_obj_align(textarea, parent, LV_ALIGN_IN_TOP_MID, 0, 0);
	lv_ta_set_text(textarea, "");
	lv_ta_set_cursor_type(textarea, LV_CURSOR_HIDDEN);

	_logs = textarea;
}


static void	createStatesView(void *tab)
{
	lv_obj_t	*parent = (lv_obj_t *)tab;
	lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
	lv_page_set_scrl_fit(parent, false, true);
	lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
	lv_style_t	*style = lv_obj_get_style(parent);
	style->body.padding.hor = 0;

	static lv_style_t style_line;
	lv_style_copy(&style_line, &lv_style_pretty_color);
	style_line.line.color = LV_COLOR_MAKE(0x56, 0x56, 0x56);
	style_line.line.width = 2;

	static lv_point_t	points[] = {{0, 0}, {0, 180}};
	lv_obj_t	*line = lv_line_create(parent, NULL);
	lv_obj_set_style(line, &style_line);
	lv_line_set_points(line, points, 2);
	lv_obj_align(line, parent, LV_ALIGN_CENTER, 0, 0);

	static lv_style_t style_led;
	lv_style_copy(&style_led, &lv_style_pretty_color);
	style_led.body.radius = LV_RADIUS_CIRCLE;
	style_led.body.main_color = LV_COLOR_MAKE(0xb5, 0x0f, 0x04);
	style_led.body.grad_color = LV_COLOR_MAKE(0x50, 0x07, 0x02);
	style_led.body.border.color = LV_COLOR_MAKE(0xfa, 0x0f, 0x00);
	style_led.body.border.width = 3;
	style_led.body.border.opa = LV_OPA_20;
	style_led.body.shadow.color = LV_COLOR_MAKE(0xb5, 0x0f, 0x04);
	style_led.body.shadow.width = 5;

	//Wifi

	lv_obj_t	*lblWifi = lv_label_create(parent, NULL);
	lv_label_set_text(lblWifi, SYMBOL_WIFI" Wifi:");
	lv_obj_align(lblWifi, parent, LV_ALIGN_IN_TOP_LEFT, 15, lv_obj_get_height(parent) / 2 - 38);

	lv_obj_t	*ledWifi = lv_led_create(parent, NULL);
	lv_obj_set_style(ledWifi, &style_led);
	lv_obj_set_size(ledWifi, 20, 20);
	lv_obj_align(ledWifi, parent, LV_ALIGN_IN_TOP_MID, -15, lv_obj_get_height(parent) / 2 - 38);
	lv_led_off(ledWifi);

	//Mqtt

	lv_obj_t	*lblMqtt = lv_label_create(parent, NULL);
	lv_label_set_text(lblMqtt, SYMBOL_UPLOAD" Mqtt:");
	lv_obj_align(lblMqtt, parent, LV_ALIGN_IN_TOP_LEFT, 15, lv_obj_get_height(parent) / 2 - 13);

	lv_obj_t	*ledMqtt = lv_led_create(parent, NULL);
	lv_obj_set_style(ledMqtt, &style_led);
	lv_obj_set_size(ledMqtt, 20, 20);
	lv_obj_align(ledMqtt, parent, LV_ALIGN_IN_TOP_MID, -15, lv_obj_get_height(parent) / 2 - 13);
	lv_led_set_bright(ledMqtt, 190);

	//Sensors

	lv_obj_t	*lblSensors = lv_label_create(parent, NULL);
	lv_label_set_text(lblSensors, SYMBOL_REFRESH" Sensors:");
	lv_obj_align(lblSensors, parent, LV_ALIGN_IN_TOP_LEFT, 15, lv_obj_get_height(parent) / 2 + 13);

	lv_obj_t	*ledSensors = lv_led_create(parent, NULL);
	lv_obj_set_style(ledSensors, &style_led);
	lv_obj_set_size(ledSensors, 20, 20);
	lv_obj_align(ledSensors, parent, LV_ALIGN_IN_TOP_MID, -15, lv_obj_get_height(parent) / 2 + 13);
	lv_led_on(ledSensors);

	//Details states

	lv_obj_t	*lblTemperature = lv_label_create(parent, NULL);
	lv_label_set_text(lblTemperature, "Temperature "SYMBOL_CLOSE);
	lv_obj_align(lblTemperature, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2 - 50);

	lv_obj_t	*lblHumidity = lv_label_create(parent, NULL);
	lv_label_set_text(lblHumidity, "Humidity "SYMBOL_CLOSE);
	lv_obj_align(lblHumidity, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2 - 25);

	lv_obj_t	*lblPressure = lv_label_create(parent, NULL);
	lv_label_set_text(lblPressure, "Pessure "SYMBOL_CLOSE);
	lv_obj_align(lblPressure, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2);

	lv_obj_t	*lblColor = lv_label_create(parent, NULL);
	lv_label_set_text(lblColor, "Color "SYMBOL_CLOSE);
	lv_obj_align(lblColor, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2 + 25);
}

static void	drawDisplayer(lv_obj_t *parent, const char *title, int x, int y, lv_style_t *styleContent, lv_style_t *styleBubble)
{
	static lv_style_t	header;
	lv_style_copy(&header, &lv_style_plain);
	header.line.color = LV_COLOR_BLACK;
	header.line.width = 19;
	header.line.rounded = 1;

	styleContent->line.width = header.line.width;
	styleContent->line.rounded = header.line.rounded;

	static lv_point_t line_points[] = {{0, 0}, {70, 0}};

	lv_obj_t	*line1 = lv_line_create(parent, NULL);
	lv_line_set_points(line1, line_points, 2);
	lv_obj_set_style(line1, &header);
	lv_obj_set_size(line1, (lv_obj_get_width(parent) - 100) / 2, 15);
	lv_obj_align(line1, parent, LV_ALIGN_IN_TOP_LEFT, 125 + x, 15 + y);

	static lv_style_t	topStyle;
	lv_style_copy(&topStyle, &lv_style_plain);
	topStyle.text.color = LV_COLOR_WHITE;
	topStyle.text.font = &lv_font_dejavu_10;

	lv_obj_t	*top = lv_label_create(line1, NULL);
	lv_obj_align(top, line1, LV_ALIGN_IN_LEFT_MID, 20, 0);
	lv_label_set_text(top, title);
	lv_label_set_style(top, &topStyle);

	lv_obj_t	*line2 = lv_line_create(parent, line1);
	lv_obj_align(line2, parent, LV_ALIGN_IN_TOP_LEFT, 125 + x, 16 + header.line.width + y);
	lv_obj_set_style(line2, styleContent);

	styleBubble->line.width = lv_obj_get_height(parent);

	lv_obj_t	*Bubble = lv_arc_create(parent, NULL);
	lv_arc_set_style(Bubble, LV_ARC_STYLE_MAIN, styleBubble);
	lv_arc_set_angles(Bubble, 0, 360);
	lv_obj_set_size(Bubble, 40, 40);
	lv_obj_align(Bubble, parent, LV_ALIGN_IN_TOP_LEFT, 105 + x, 5 + y);
}

static void	createSensorsView(void *tab)
{
	lv_obj_t	*parent = (lv_obj_t *)tab;
	lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
	lv_page_set_scrl_fit(parent, false, true);
	lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
	lv_style_t	*style = lv_obj_get_style(parent);
	style->body.padding.hor = 0;

	//Menu

	lv_obj_t	*listMenu = lv_list_create(parent, NULL);

	static lv_style_t style_btn_rel;
	lv_style_copy(&style_btn_rel, &lv_style_btn_rel);
	style_btn_rel.body.main_color = LV_COLOR_MAKE(0x30, 0x30, 0x30);
	style_btn_rel.body.grad_color = LV_COLOR_BLACK;
	style_btn_rel.body.border.color = LV_COLOR_SILVER;
	style_btn_rel.body.border.width = 1;
	style_btn_rel.body.border.opa = LV_OPA_50;
	style_btn_rel.body.radius = 0;
	style_btn_rel.body.padding.hor = 10;

	lv_obj_set_size(listMenu, 100, lv_obj_get_height(parent) + 5);
	lv_list_set_sb_mode(listMenu, LV_SB_MODE_AUTO);
	lv_obj_align(listMenu, parent, LV_ALIGN_IN_TOP_LEFT, 0, -5);

	lv_obj_t *tmp  = lv_list_add(listMenu, SYMBOL_LIST, "All", NULL);
	lv_list_add(listMenu, NULL, "Temperature", NULL);
	lv_list_add(listMenu, NULL, "Humidity", NULL);
	lv_list_add(listMenu, NULL, "Pressure", NULL);
	lv_list_add(listMenu, NULL, "Color", NULL);

	lv_list_set_style(listMenu, LV_LIST_STYLE_BG, &lv_style_transp_tight);
	lv_list_set_style(listMenu, LV_LIST_STYLE_BTN_REL, &style_btn_rel);
	lv_list_set_sb_mode(listMenu, LV_SB_MODE_HIDE);

	lv_btn_set_state(tmp, LV_BTN_STATE_PR);

	//Color displayer

	static lv_style_t styleBlue;
	lv_style_copy(&styleBlue, &lv_style_plain);
	styleBlue.line.color = LV_COLOR_BLUE;           /*Arc color*/
	styleBlue.line.width = lv_obj_get_height(parent);                       /*Arc width*/

	static lv_style_t styleGreen;
	lv_style_copy(&styleGreen, &styleBlue);
	styleGreen.line.color = LV_COLOR_GREEN;    /*Arc color*/

	static lv_style_t styleRed;
	lv_style_copy(&styleRed, &styleBlue);
	styleRed.line.color = LV_COLOR_RED;  /*Arc color*/

	/*Create an Arc*/
	lv_obj_t	*arcBlue = lv_arc_create(parent, NULL);
	lv_arc_set_style(arcBlue, LV_ARC_STYLE_MAIN, &styleBlue);          /*Use the new style*/
	lv_arc_set_angles(arcBlue, 180, 210);
	lv_obj_set_size(arcBlue, lv_obj_get_width(parent), lv_obj_get_height(parent));
	lv_obj_align(arcBlue, parent, LV_ALIGN_IN_BOTTOM_RIGHT, lv_obj_get_width(parent) / 2, lv_obj_get_height(parent) / 2);

	/*Create an Arc*/
	lv_obj_t	*arcGreen = lv_arc_create(parent, arcBlue);
	lv_arc_set_style(arcGreen, LV_ARC_STYLE_MAIN, &styleGreen);          /*Use the new style*/
	lv_arc_set_angles(arcGreen, 210, 240);

	/*Create an Arc*/
	lv_obj_t	*arcRed = lv_arc_create(parent, arcBlue);
	lv_arc_set_style(arcRed, LV_ARC_STYLE_MAIN, &styleRed);          /*Use the new style*/
	lv_arc_set_angles(arcRed, 240, 270);

	//Displayer temperature
	static lv_style_t	styleTemmperature;
	lv_style_copy(&styleTemmperature, &lv_style_plain);
	styleTemmperature.line.color = LV_COLOR_RED;

	static lv_style_t	styleTemmperatureBubble;
	lv_style_copy(&styleTemmperatureBubble, &lv_style_plain);
	styleTemmperatureBubble.line.color = LV_COLOR_RED;
	drawDisplayer(parent, "Temperature", 0, 0, &styleTemmperature, &styleTemmperatureBubble);

	static lv_style_t	styleHumidity;
	lv_style_copy(&styleHumidity, &lv_style_plain);
	styleHumidity.line.color = LV_COLOR_GREEN;

	static lv_style_t	styleHumidityBubble;
	lv_style_copy(&styleHumidityBubble, &lv_style_plain);
	styleHumidityBubble.line.color = LV_COLOR_GREEN;
	drawDisplayer(parent, "Humidity", (lv_obj_get_width(parent) - 100) / 2, 0, &styleHumidity, &styleHumidityBubble);

	static lv_style_t	stylePressure;
	lv_style_copy(&stylePressure, &lv_style_plain);
	stylePressure.line.color = LV_COLOR_BLUE;

	static lv_style_t	stylePressureBubble;
	lv_style_copy(&stylePressureBubble, &lv_style_plain);
	stylePressureBubble.line.color = LV_COLOR_BLUE;
	drawDisplayer(parent, "Pressure", 0, 45, &stylePressure, &stylePressureBubble);
}

static void	setupUI()
{
	ESP_LOGI(TAG, "Drawing general UI...");

	lv_theme_t	*th = lv_theme_material_init(0, NULL);
	lv_theme_set_current(th);

	lv_obj_t * tv = lv_tabview_create(lv_scr_act(), NULL);                               /*Create a slider*/
	lv_style_t	*paddingOff = lv_obj_get_style(tv);
	paddingOff->body.padding.inner = 0;
	paddingOff->body.padding.ver = 0;
	paddingOff->body.padding.hor = 0;

	lv_obj_t * tab1 = lv_tabview_add_tab(tv, "Sensors");
	lv_obj_t * tab2 = lv_tabview_add_tab(tv, "Logs");
	lv_obj_t * tab3 = lv_tabview_add_tab(tv, "States");
	
	createSensorsView(tab1);
	createLogsView(tab2);
	createStatesView(tab3);
	_tv = tv;
}

static void taskLcd(void *args)
{
    ESP_LOGI(TAG, "Initiating the task...");
	while(_running) {
		_working = true;
	}
	_working = false;
	deinitLcd();
	lcdTask = NULL;
	vTaskDelete(NULL);
}

//Public function

static void	changePage(int i)
{
	static int8_t	index = 0;

	index += i;
	if (index < 0)
		index = index % 3 + 3;
	if (index > 3)
		index = 0;
	if (_tv != NULL && xSemaphoreTake(_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE){
		lv_tabview_set_tab_act(_tv, index, true);
		xSemaphoreGive(_semaphore);
	}
}

void	nextPage()
{
	changePage(1);
}

void	previousPage()
{
	changePage(-1);
}

esp_err_t	startLcd()
{
	ESP_LOGI(TAG, "Starting the %s task...", TAG);
	if (_running)
		return ESP_FAIL;
	if (!_semaphore)
		_semaphore = xSemaphoreCreateMutex();
	_running = true;
	ESP_ERROR_CHECK(initLcd());
	setupUI();
	return xTaskCreate(&taskLcd, "lcdTask", 4098, NULL, tskIDLE_PRIORITY, &lcdTask);
}

esp_err_t	stopLcd()
{
	ESP_LOGI(TAG, "Stopping the %s task...", TAG);
	if (!_running)
		return ESP_FAIL;
	_running = false;
	return ESP_OK;
}

char	lcdIsRunning()
{
	return _running;
}

char	lcdIsWorking()
{
	return _working;
}

SemaphoreHandle_t	lcdGetSemaphore()
{
	return _semaphore;
}