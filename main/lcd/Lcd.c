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

static int	customVPrintF(const char *str, va_list arg)
{
    static char	*logs[15];
    static char	**next = logs;
    static uint8_t	index = 0;
    lv_obj_t    *log = getLogTa();

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
static void IRAM_ATTR lv_tick_task(void)
{
    lv_tick_inc(portTICK_RATE_MS);
}

static esp_err_t	initLcd()
{
    esp_err_t	ret = ESP_OK;

    ESP_LOGI(TAG, "Initiating the LCD...");
    if (xSemaphoreTake(_semaphore, 10) != pdTRUE)
        return ESP_FAIL;
    ili9341_init();

    esp_log_set_vprintf(&customVPrintF);

    lv_disp_drv_t disp;
    lv_disp_drv_init(&disp);
    disp.disp_flush = ili9341_flush;
    disp.disp_fill = ili9341_fill;
    disp.disp_map = ili9341_map;
    ret = lv_disp_drv_register(&disp) != NULL ? ESP_OK : ESP_FAIL; 
    esp_register_freertos_tick_hook(lv_tick_task);
    xSemaphoreGive(_semaphore);   
    return ret;
}

static esp_err_t	deinitLcd()
{
    ESP_LOGI(TAG, "Deinitiating the LCD...");
    if (xSemaphoreTake(_semaphore, 10) != pdTRUE)
        return ESP_FAIL;
    esp_log_set_vprintf(&vprintf);
    ili9341_deinit();
    //esp_deregister_freertos_idle_hook_for_cpu(lv_tick_task, xPortGetCoreID());
    xSemaphoreGive(_semaphore);
    //return disp_spi_deinit();
    return ESP_OK;
}

/* 
 * General UI setter
*/

static void	setupUI()
{
    ESP_LOGI(TAG, "Drawing general UI...");

    if (_first){
        if (xSemaphoreTake(_semaphore, 10) != pdTRUE)
            return;
        #if USE_LV_THEME_DEFAULT == 1
        lv_theme_t	*th = lv_theme_default_init(0, NULL);
        #else
        lv_theme_t	*th = lv_theme_material_init(0, NULL);
        #endif
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
        xSemaphoreGive(_semaphore);
    } else
        ESP_LOGI(TAG, "Already drawed");
}

static void refreshSensors(SensorValues_t *values)
{
    switch (getSensorsCurrentPage())
    {
        case ALL:
        if (values->temperature != -1) {
            if (sensorsData->temperature->value && sensorsData->temperature->decimale && xSemaphoreTake(_semaphore, 10) == pdTRUE) {
                sprintf(_buffer, "%d", (int)values->temperature);
                lv_label_set_text(sensorsData->temperature->value, _buffer);
                sprintf(_buffer, ",%d°C", (int)(values->temperature * 100) % 100);
                lv_label_set_text(sensorsData->temperature->decimale, _buffer);
                xSemaphoreGive(_semaphore);
            }
        }
        if (values->humidity != -1) {
            if (sensorsData->humidity->value && sensorsData->humidity->decimale && xSemaphoreTake(_semaphore, 10) == pdTRUE) {
                sprintf(_buffer, "%d", (int)values->humidity);
                lv_label_set_text(sensorsData->humidity->value, _buffer);
                sprintf(_buffer, ",%d%c", (int)(values->humidity * 100) % 100, '%');
                lv_label_set_text(sensorsData->humidity->decimale, _buffer);
                xSemaphoreGive(_semaphore);
            }
        }
        if (values->pressure != -1) {
            if (sensorsData->pressure->value && sensorsData->pressure->decimale && xSemaphoreTake(_semaphore, 10) == pdTRUE) {
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
            if (xSemaphoreTake(_semaphore, 10) == pdTRUE) {
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
        sprintf(_buffer, "%d", values->battery);
        lv_label_set_text(sensorsData->battery->value, _buffer);
        break;

        case TEMPERATURE:
        if (sensorsData->meterLblTemperature && 
            sensorsData->meterTemperature && 
            sensorsData->chartSerTemperature && 
            sensorsData->chartTemperature && 
            xSemaphoreTake(_semaphore, 10) == pdTRUE) {
            lv_gauge_set_value(sensorsData->meterTemperature, 0, (int)values->temperature);
            sprintf(_buffer, "%d,%d°C", (int)values->temperature, (int)(values->temperature * 100) % 100);
            lv_label_set_text(sensorsData->meterLblTemperature, _buffer);
            for (int i = 0;i < 9;i++){
                sensorsData->chartSerTemperature->points[i] = sensorsData->chartSerTemperature->points[i + 1];
            }
            sensorsData->chartSerTemperature->points[9] = (int)values->temperature;
            lv_chart_refresh(sensorsData->chartTemperature);
            xSemaphoreGive(_semaphore);
        }
        break;

        case HUMIDITY:
        if (values->humidity != -1) {
            if (sensorsData->meterLblHumidity && 
            sensorsData->meterHumidity && 
            sensorsData->chartSerHumidity && 
            sensorsData->chartHumidity && 
            xSemaphoreTake(_semaphore, 10) == pdTRUE) {
                lv_gauge_set_value(sensorsData->meterHumidity, 0, (int)values->humidity);
                sprintf(_buffer, "%d,%d%c", (int)values->humidity, (int)(values->humidity * 100) % 100, '%');
                lv_label_set_text(sensorsData->meterLblHumidity, _buffer);
                for (int i = 0;i < 9;i++){
                    sensorsData->chartSerHumidity->points[i] = sensorsData->chartSerHumidity->points[i + 1];
                }
                sensorsData->chartSerHumidity->points[9] = (int)values->humidity;
                lv_chart_refresh(sensorsData->chartHumidity);
                xSemaphoreGive(_semaphore);
            }
        }
        break;

        case PRESSURE:
        if (values->pressure != -1) {
            if (sensorsData->meterLblPressure && 
            sensorsData->chartSerPressure && 
            sensorsData->chartPressure && 
            xSemaphoreTake(_semaphore, 10) == pdTRUE) {
                sprintf(_buffer, "%d,%dhPa", (int)values->pressure, (int)(values->pressure * 100) % 100);
                lv_label_set_text(sensorsData->meterLblPressure, _buffer);
                for (int i = 0;i < 29;i++){
                    sensorsData->chartSerPressure->points[i] = sensorsData->chartSerPressure->points[i + 1];
                }
                sensorsData->chartSerPressure->points[29] = (int)(values->pressure * 100) % 100;
                lv_chart_refresh(sensorsData->chartPressure);
                xSemaphoreGive(_semaphore);
            }
        }
        break;

        default:
        break;
    }
}

static void refreshState(SensorValues_t *values)
{
    if (xSemaphoreTake(_semaphore, 10) == pdTRUE) {
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
    ESP_ERROR_CHECK(initLcd());
    setupUI();
    while(_running) {
        _working = true;
        if (_tv != NULL && xSemaphoreTake(_semaphore, 10) == pdTRUE) {
            if (lv_tabview_get_tab_act(_tv) != _index) {
                    lv_tabview_set_tab_act(_tv, _index, true);
            }
            xSemaphoreGive(_semaphore);
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
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    deinitLcd();
    _working = false;
    vTaskDelete(lcdTask);
}

static void	changePage(int i)
{
    _index += i;
    if (_index < 0)
        _index = _index + 3;
    if (_index >= 3)
        _index = 0;
}

/* 
 * Public function
*/

int32_t getCurrentPage()
{
    return _index;
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
    ESP_LOGI(TAG, "Starting the %s task...", TAG);
    if (!_semaphore)
        _semaphore = xSemaphoreCreateMutex();
    _running = true;
    return xTaskCreate(&taskLcd, "lcdTask", 6122, NULL, tskIDLE_PRIORITY, &lcdTask);
}

esp_err_t	stopLcd()
{
    if (!_running)
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

void    updateToNewFirmware(uint8_t value, const char *version, const char show)
{
    static lv_obj_t    *contentUpdate = NULL;
    static lv_obj_t    *pbUpdate = NULL;
    static lv_obj_t    *txtUpdate = NULL;
    static const char  *oldVersion = NULL;
    static uint8_t      oldValue = 0;
    
    if (version)
        oldVersion = version;
    if (!_running)
        return;
    if (xSemaphoreTake(_semaphore, 10) != pdTRUE)
        return;
    if (show && !contentUpdate) {
        contentUpdate = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_align(contentUpdate, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
        lv_cont_set_fit(contentUpdate, true, true);
        lv_cont_set_layout(contentUpdate, LV_LAYOUT_CENTER);
        lv_obj_t    *txtUpdating = lv_label_create(contentUpdate, NULL);
        lv_obj_align(txtUpdating, contentUpdate, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_label_set_text(txtUpdating, "Updating firmware...");
        pbUpdate = lv_bar_create(contentUpdate, NULL);
        lv_obj_align(pbUpdate, contentUpdate, LV_ALIGN_IN_TOP_MID, 0, 0);
        lv_bar_set_range(pbUpdate, 0, 100);
        lv_bar_set_value(pbUpdate, 0);
        txtUpdate = lv_label_create(pbUpdate, NULL);
        lv_obj_align(txtUpdate, pbUpdate, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(txtUpdate, "0%");
        if (oldVersion){
            lv_obj_t    *txtFirmware = lv_label_create(contentUpdate, NULL);
            static lv_style_t   txtStyle;
            lv_style_copy(&txtStyle, lv_obj_get_style(txtFirmware));
            txtStyle.text.font = &lv_font_dejavu_10;
            lv_obj_align(txtUpdating, contentUpdate, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
            lv_obj_set_style(txtFirmware, &txtStyle);
            sprintf(_buffer, "%s -> %s", CURRENT_VERSION, version);
            lv_label_set_text(txtFirmware, _buffer);
        }
        oldValue = 0;
    } else if (!show && contentUpdate) {
        lv_obj_del(contentUpdate);
        contentUpdate = NULL;
        pbUpdate = NULL;
        txtUpdate = NULL;
    }
    if (pbUpdate && oldValue != value) {
        lv_bar_set_value(pbUpdate, value);
        sprintf(_buffer, "%d%c", value, '%');
        lv_label_set_text(txtUpdate, _buffer);
        oldValue = value;
    }
    xSemaphoreGive(_semaphore);
}