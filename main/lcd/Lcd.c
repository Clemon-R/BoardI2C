#include "Lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "freertos/event_groups.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"

#include "../wifi/WifiClient.h"
#include "../mqtt/MqttClient.h"
#include "../sensors/SensorClient.h"
#include "../ButtonClient.h"

#include "../../drv/disp_spi.h"
#include "../../drv/ili9341.h"

#include <math.h>

static const char	*TAG = "\033[1;93mLcd\033[0m";
static TaskHandle_t	lcdTask = NULL;

//Working var
static char	_running = false;
static char	_working = false;
static char _first = true;
static SemaphoreHandle_t	_semaphore = NULL;

//The actual data
static LcdSensors_t *sensorsData = NULL;
static LcdState_t *stateData = NULL;

//Tabview the main menu
static lv_obj_t	*_tv = NULL;

//Pos of the view
static int32_t	_index = 0;

static char	_buffer[BUFF_SIZE];

static void IRAM_ATTR lv_tick_task(void)
{
    lv_tick_inc(portTICK_RATE_MS);
}

static int	customVPrintF(const char *str, va_list arg)
{
    static char	**logs = NULL;
    static char	**next = NULL;
    static uint8_t	index = 0;
    lv_obj_t    *log = getLogTa();

    if (!logs) {
        logs = (char **)malloc(sizeof(char *) * 15);
        memset(logs, 0, sizeof(char *) * 15);
        next = logs;
    }
    if (log) {
        vsprintf(_buffer, str, arg);
        if (!*next) {
            *next = strdup(_buffer);
            if (index < 13) {
                index++;
                next = logs + index;
            }
        } else {
            for (int i = 0; i < index; i++) {
                if (i == 0) {
                    free(logs[i]);
                }
                logs[i] = logs[i + 1];
            }
            *next = strdup(_buffer);
        }
        if (xSemaphoreTake(_semaphore, pdMS_TO_TICKS(1000)) == pdTRUE) {
            lv_ta_set_text(log, "");
            for (int i = 0; logs && logs[i]; i++) {
                lv_ta_add_text(log, logs[i]);
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
    ili9341_deinit();
    lv_disp_set_active(NULL);
    return disp_spi_deinit();
}

/* 
 * General UI setter
*/

static void	setupUI()
{
    ESP_LOGI(TAG, "Drawing general UI...");

    if (_first){
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

        sensorsData = getSensorsData();
        stateData = getStateData();

        createSensorsView(tab1);
        createLogsView(tab2);
        createStatesView(tab3);
        _tv = tv;
        _first = false;
    } else
        ESP_LOGI(TAG, "Already drawed");
}

static void refreshSensors(SensorValues_t *values)
{
    if (values->temperature != -1) {
        if (sensorsData->temperature->value && sensorsData->temperature->decimale && xSemaphoreTake(_semaphore, 0) == pdTRUE) {
            lv_lmeter_set_value(sensorsData->meterTemperature, (int)values->temperature);
            sprintf(_buffer, "%d", (int)values->temperature);
            lv_label_set_text(sensorsData->temperature->value, _buffer);
            sprintf(_buffer, ",%d°C", (int)(values->temperature * 100) % 100);
            lv_label_set_text(sensorsData->temperature->decimale, _buffer);
            sprintf(_buffer, "%d,%d°C", (int)values->temperature, (int)(values->temperature * 100) % 100);
            lv_label_set_text(sensorsData->meterLblTemperature, _buffer);
            xSemaphoreGive(_semaphore);
        }
    }
    if (values->humidity != -1) {
        if (sensorsData->humidity->value && sensorsData->humidity->decimale && xSemaphoreTake(_semaphore, 0) == pdTRUE) {
            sprintf(_buffer, "%d", (int)values->humidity);
            lv_label_set_text(sensorsData->humidity->value, _buffer);
            sprintf(_buffer, ",%d%c", (int)(values->humidity * 100) % 100, '%');
            lv_label_set_text(sensorsData->humidity->decimale, _buffer);
            xSemaphoreGive(_semaphore);
        }
    }
    if (values->pressure != -1) {
        if (sensorsData->pressure->value && sensorsData->pressure->decimale && xSemaphoreTake(_semaphore, 0) == pdTRUE) {
            sprintf(_buffer, "%d", (int)values->pressure);
            lv_label_set_text(sensorsData->pressure->value, _buffer);
            sprintf(_buffer, ",%dhPa", (int)(values->pressure * 100) % 100);
            lv_label_set_text(sensorsData->pressure->decimale, _buffer);
            xSemaphoreGive(_semaphore);
        }
    }

    uint32_t	total = values->color.r + values->color.g + values->color.b;
    uint32_t	last = 270, tmp = 0;
    if (values->color.available) {
        if (xSemaphoreTake(_semaphore, 0) == pdTRUE) {
            if (sensorsData->red) {
                lv_arc_set_angles(sensorsData->red, (tmp = last - (90 * (values->color.r / (float)total))), last);
                last = tmp;
            }
            if (sensorsData->green) {
                lv_arc_set_angles(sensorsData->green, (tmp = last - (90 * (values->color.g / (float)total))), last);
                last = tmp;
            }
            if (sensorsData->blue) {
                lv_arc_set_angles(sensorsData->blue, (tmp = last - (90 * (values->color.b / (float)total))), last);
            }
            if (sensorsData->rgb) {
                lv_arc_get_style(sensorsData->rgb, LV_ARC_STYLE_MAIN)->line.color = LV_COLOR_MAKE((int)(UINT8_MAX * (values->color.r / (float)UINT16_MAX)), (int)(UINT8_MAX * (values->color.g  / UINT16_MAX)), (int)(UINT8_MAX * (values->color.b  / UINT16_MAX)));
            }
            xSemaphoreGive(_semaphore);
        }
    }
}

static void refreshState(SensorValues_t *values)
{
    if (xSemaphoreTake(_semaphore, 0) == pdTRUE) {
        ESP_LOGI(TAG, "temperature: %f, humidity: %f, pressure: %f, color: %d, state: %d", values->temperature, values->humidity, values->pressure, values->color.available, values->initiated);
        if (values->temperature != -1)
            lv_label_set_text(stateData->lblTemperature, "Temperature "SYMBOL_OK);
        else
            lv_label_set_text(stateData->lblTemperature, "Temperature "SYMBOL_CLOSE);

        if (values->humidity != -1)
            lv_label_set_text(stateData->lblHumidity, "Humidity "SYMBOL_OK);
        else
            lv_label_set_text(stateData->lblHumidity, "Humidity "SYMBOL_CLOSE);

        if (values->pressure != -1)
            lv_label_set_text(stateData->lblPressure, "Pessure "SYMBOL_OK);
        else
            lv_label_set_text(stateData->lblPressure, "Pessure "SYMBOL_CLOSE);

        if (values->color.available)
            lv_label_set_text(stateData->lblColor, "Color "SYMBOL_OK);
        else
            lv_label_set_text(stateData->lblColor, "Color "SYMBOL_CLOSE);
        if (isWifiConnected())
            lv_led_on(stateData->ledWifi);
        else
            lv_led_off(stateData->ledWifi);

        if (isMqttConnected())
            lv_led_on(stateData->ledMqtt);
        else
            lv_led_off(stateData->ledMqtt);

        if (values->temperature != -1 && values->humidity != -1 && values->pressure != -1 && values->color.available)
            lv_led_set_bright(stateData->ledSensors, 255);
        else if (values->temperature != -1 || values->humidity != -1 || values->pressure != -1 || values->color.available)
            lv_led_set_bright(stateData->ledSensors, 170);
        else
            lv_led_set_bright(stateData->ledSensors, 0);
        xSemaphoreGive(_semaphore);
    }
}

/* 
 * Running Task
*/

static void taskLcd(void *args)
{
    SensorValues_t  values;

    ESP_LOGI(TAG, "Initiating the task...");
    while(_running) {
        _working = true;
        if (lv_tabview_get_tab_act(_tv) != _index) {
            if (_tv != NULL && xSemaphoreTake(_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
                lv_tabview_set_tab_act(_tv, _index, true);
                xSemaphoreGive(_semaphore);
            }
        }
        values = getSensorValues();
        if (!values.initiated && _index == 0)
            continue;
        switch (_index) {
        case 0:
            refreshSensors(&values);
            break;
        case 2:
            refreshState(&values);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
    _working = false;
    _running = false;
    deinitLcd();
    lcdTask = NULL;
    vTaskDelete(NULL);
}

/* 
 * Public function
*/

static void	changePage(int i)
{
    _index += i;
    if (_index < 0)
        _index = _index + 3;
    if (_index >= 3)
        _index = 0;
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
    if (_running)
        return ESP_FAIL;
    if (startButtonClient() == ESP_FAIL)
        return ESP_FAIL;
    ESP_LOGI(TAG, "Starting the %s task...", TAG);
    if (!_semaphore)
        _semaphore = xSemaphoreCreateMutex();
    _running = true;
    ESP_ERROR_CHECK(initLcd());
    setupUI();
    return xTaskCreate(&taskLcd, "lcdTask", 4096, NULL, tskIDLE_PRIORITY, &lcdTask);
}

esp_err_t	stopLcd()
{
    if (!_running)
        return ESP_FAIL;
    if (stopButtonClient() == ESP_FAIL)
        return ESP_FAIL;
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
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