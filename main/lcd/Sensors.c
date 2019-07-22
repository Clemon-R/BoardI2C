#include "Sensors.h"

static const char   *TAG = "LcdSensors";

static ValueContainer_t temperature = {.value = NULL, .decimale = NULL};
static ValueContainer_t humidity = {.value = NULL, .decimale = NULL};
static ValueContainer_t pressure = {.value = NULL, .decimale = NULL};

static LcdSensors_t data;

static void	drawDisplayer(lv_obj_t *parent, const char *title, int x, int y, lv_style_t *styleContent, lv_style_t *styleBubble, lv_obj_t **value, lv_obj_t **decimal)
{
    static lv_style_t	header;
    lv_style_copy(&header, &lv_style_plain);
    header.line.color = LV_COLOR_BLACK;
    header.line.width = 19;
    header.line.rounded = 1;
    header.body.padding.ver = 0;

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

    lv_obj_t	*top = lv_label_create(parent, NULL);
    lv_obj_align(top, line1, LV_ALIGN_IN_LEFT_MID, 20, 0);
    lv_label_set_text(top, title);
    lv_label_set_style(top, &topStyle);

    lv_obj_t	*line2 = lv_line_create(parent, line1);
    lv_obj_align(line2, parent, LV_ALIGN_IN_TOP_LEFT, 125 + x, 16 + header.line.width + y);
    lv_obj_set_style(line2, styleContent);

    styleBubble->line.width = lv_obj_get_height(parent);

    lv_obj_t	*bubble = lv_arc_create(parent, NULL);
    lv_arc_set_style(bubble, LV_ARC_STYLE_MAIN, styleBubble);
    lv_arc_set_angles(bubble, 0, 360);
    lv_obj_set_size(bubble, 40, 40);
    lv_obj_align(bubble, parent, LV_ALIGN_IN_TOP_LEFT, 105 + x, 5 + y);
    if (value != NULL) {
        *value = lv_label_create(parent, NULL);
        lv_obj_align(*value, bubble, LV_ALIGN_CENTER, 8, 0);
    }
    if (decimal != NULL) {
        *decimal = lv_label_create(parent, NULL);
        lv_obj_align(*decimal, line2, LV_ALIGN_IN_LEFT_MID, 20, 0);
    }
}

static void createSensorsAllView(void *tab)
{
    lv_obj_t	*parent = (lv_obj_t *)tab;
    lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
    lv_page_set_scrl_fit(parent, false, false);
    lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
    lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
	lv_obj_set_height(parent, lv_obj_get_height(parent) - 40);
    static lv_style_t	style;
	lv_style_copy(&style, lv_obj_get_style(parent));
    style.body.padding.hor = -10;
    style.body.padding.ver = -10;
    style.body.padding.inner = 0;
	lv_obj_set_style(parent, &style);

    //Color displayer
    static lv_style_t styleBlue;
    lv_style_copy(&styleBlue, &lv_style_plain);
    styleBlue.line.color = LV_COLOR_BLUE;           /*Arc color*/
    styleBlue.line.width = lv_obj_get_height(parent) / 2;                       /*Arc width*/

    static lv_style_t styleGreen;
    lv_style_copy(&styleGreen, &styleBlue);
    styleGreen.line.color = LV_COLOR_GREEN;    /*Arc color*/

    static lv_style_t styleRed;
    lv_style_copy(&styleRed, &styleBlue);
    styleRed.line.color = LV_COLOR_RED;  /*Arc color*/

    /*Create an Arc*/
    data.blue = lv_arc_create(parent, NULL);
    lv_arc_set_style(data.blue, LV_ARC_STYLE_MAIN, &styleBlue);          /*Use the new style*/
    lv_arc_set_angles(data.blue, 180, 210);
    lv_obj_set_size(data.blue, lv_obj_get_width(parent), lv_obj_get_height(parent));
    lv_obj_align(data.blue, parent, LV_ALIGN_IN_BOTTOM_RIGHT, lv_obj_get_width(parent) / 2, lv_obj_get_height(parent) / 2);

    /*Create an Arc*/
    data.green = lv_arc_create(parent, data.blue);
    lv_arc_set_style(data.green, LV_ARC_STYLE_MAIN, &styleGreen);          /*Use the new style*/
    lv_arc_set_angles(data.green, 210, 240);

    /*Create an Arc*/
    data.red = lv_arc_create(parent, data.blue);
    lv_arc_set_style(data.red, LV_ARC_STYLE_MAIN, &styleRed);          /*Use the new style*/
    lv_arc_set_angles(data.red, 240, 270);

    static lv_style_t styleRGB;
    lv_style_copy(&styleRGB, &styleBlue);
    styleRGB.line.color = LV_COLOR_RED;
    styleRGB.line.width = 10;

    data.rgb = lv_arc_create(parent, NULL);
    lv_arc_set_style(data.rgb, LV_ARC_STYLE_MAIN, &styleRGB);
    lv_arc_set_angles(data.rgb, 180, 270);
    lv_obj_set_size(data.rgb, lv_obj_get_width(parent) * 1.1f, lv_obj_get_height(parent) * 1.1f);
    lv_obj_align(data.rgb, parent, LV_ALIGN_IN_BOTTOM_RIGHT, lv_obj_get_width(parent) / 2* 1.1f, lv_obj_get_height(parent) / 2* 1.1f);

    //Displayer temperature
    static lv_style_t	valueStyle;
    lv_style_copy(&valueStyle, &lv_style_plain);
    valueStyle.text.color = LV_COLOR_WHITE;
    valueStyle.text.font = &lv_font_dejavu_20;

    static lv_style_t	decimaleStyle;
    lv_style_copy(&decimaleStyle, &lv_style_plain);
    decimaleStyle.text.color = LV_COLOR_WHITE;
    decimaleStyle.text.font = &lv_font_dejavu_10;

    static lv_style_t	styleTemmperature;
    lv_style_copy(&styleTemmperature, &lv_style_plain);
    styleTemmperature.line.color = LV_COLOR_RED;

    static lv_style_t	styleTemmperatureBubble;
    lv_style_copy(&styleTemmperatureBubble, &lv_style_plain);
    styleTemmperatureBubble.line.color = LV_COLOR_RED;
    drawDisplayer(parent, "Temperature", 0, 0, &styleTemmperature, &styleTemmperatureBubble, &data.temperature->value, &data.temperature->decimale);
    lv_label_set_text(data.temperature->value, "100");
    lv_label_set_text(data.temperature->decimale, ",00°C");
    lv_obj_set_style(data.temperature->value, &valueStyle);
    lv_obj_set_style(data.temperature->decimale, &decimaleStyle);

    static lv_style_t	styleHumidity;
    lv_style_copy(&styleHumidity, &lv_style_plain);
    styleHumidity.line.color = LV_COLOR_GREEN;

    static lv_style_t	styleHumidityBubble;
    lv_style_copy(&styleHumidityBubble, &lv_style_plain);
    styleHumidityBubble.line.color = LV_COLOR_GREEN;
    drawDisplayer(parent, "Humidity", (lv_obj_get_width(parent) - 100) / 2, 0, &styleHumidity, &styleHumidityBubble, &data.humidity->value, &data.humidity->decimale);
    lv_label_set_text(data.humidity->value, "100");
    lv_label_set_text(data.humidity->decimale, ",00°C");
    lv_obj_set_style(data.humidity->value, &valueStyle);
    lv_obj_set_style(data.humidity->decimale, &decimaleStyle);

    static lv_style_t	stylePressure;
    lv_style_copy(&stylePressure, &lv_style_plain);
    stylePressure.line.color = LV_COLOR_BLUE;

    static lv_style_t	stylePressureBubble;
    lv_style_copy(&stylePressureBubble, &lv_style_plain);
    stylePressureBubble.line.color = LV_COLOR_BLUE;
    drawDisplayer(parent, "Pressure", 0, 45, &stylePressure, &stylePressureBubble, &data.pressure->value, &data.pressure->decimale);
    lv_label_set_text(data.pressure->value, "100");
    lv_label_set_text(data.pressure->decimale, ",00°C");
    lv_obj_set_style(data.pressure->value, &valueStyle);
    lv_obj_set_style(data.pressure->decimale, &decimaleStyle);
}

static void createSensorsTemperatureView(void *tab)
{
    lv_obj_t	*parent = (lv_obj_t *)tab;
    lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
    lv_page_set_scrl_fit(parent, false, false);
    lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
    lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
	lv_obj_set_height(parent, lv_obj_get_height(parent) - 40);
    static lv_style_t	style;
	lv_style_copy(&style, lv_obj_get_style(parent));
    style.body.padding.hor = -10;
    style.body.padding.ver = -10;
    style.body.padding.inner = 0;
	lv_obj_set_style(parent, &style);

	data.chartTemperature = lv_chart_create(parent, NULL);
	data.chartSerTemperature = lv_chart_add_series(data.chartTemperature, LV_COLOR_RED);
	lv_chart_set_type(data.chartTemperature, LV_CHART_TYPE_LINE);
	lv_chart_set_point_count(data.chartTemperature, 10);
	lv_chart_set_range(data.chartTemperature, -100, 100);
	lv_chart_set_div_line_count(data.chartTemperature, 5, 8);
	lv_obj_set_width(data.chartTemperature, lv_obj_get_width(parent) - 100);
	lv_obj_align(data.chartTemperature, parent, LV_ALIGN_IN_BOTTOM_RIGHT, -3, -5);
	lv_chart_init_points(data.chartTemperature, data.chartSerTemperature, 1);
    lv_obj_t    *init = lv_label_create(parent, NULL);
    lv_label_set_text(init, "0°C");
    lv_obj_align(init, data.chartTemperature, LV_ALIGN_OUT_LEFT_MID, 0, 0);

    data.meterTemperature = lv_gauge_create(parent, NULL);
    static lv_style_t   styleMeter;
    lv_style_copy(&styleMeter, lv_obj_get_style(data.meterTemperature));
    styleMeter.body.main_color = LV_COLOR_AQUA;
    styleMeter.body.grad_color = LV_COLOR_RED;
    styleMeter.body.radius = lv_obj_get_height(parent) / 4;
    lv_gauge_set_style(data.meterTemperature, &styleMeter);
    lv_gauge_set_critical_value(data.meterTemperature, 45);
    lv_gauge_set_scale(data.meterTemperature, 180, 31, 5);
    lv_gauge_set_range(data.meterTemperature, -100, 100);
    lv_gauge_set_needle_count(data.meterTemperature, 1, &LV_COLOR_BLACK);
    lv_gauge_set_value(data.meterTemperature, 0, 0);
    lv_obj_set_width(data.meterTemperature, (lv_obj_get_width(parent) - 100) * 0.80);
	lv_obj_align(data.meterTemperature, parent, LV_ALIGN_IN_TOP_MID, 50, 0);

	data.meterLblTemperature = lv_label_create(parent, NULL);
	lv_obj_align(data.meterLblTemperature, parent, LV_ALIGN_IN_TOP_LEFT, 102, 2);
	lv_label_set_text(data.meterLblTemperature, "0,00°C");
}

static void createSensorsHumidityView(void *tab)
{
    lv_obj_t	*parent = (lv_obj_t *)tab;
    lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
    lv_page_set_scrl_fit(parent, false, false);
    lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
    lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
	lv_obj_set_height(parent, lv_obj_get_height(parent) - 40);
    static lv_style_t	style;
	lv_style_copy(&style, lv_obj_get_style(parent));
    style.body.padding.hor = -10;
    style.body.padding.ver = -10;
    style.body.padding.inner = 0;
	lv_obj_set_style(parent, &style);

    data.chartHumidity = lv_chart_create(parent, NULL);
	data.chartSerHumidity = lv_chart_add_series(data.chartHumidity, LV_COLOR_RED);
	lv_chart_set_type(data.chartHumidity, LV_CHART_TYPE_COLUMN);
	lv_chart_set_point_count(data.chartHumidity, 10);
	lv_chart_set_range(data.chartHumidity, -2, 100);
	lv_chart_set_div_line_count(data.chartHumidity, 5, 8);
	lv_obj_set_width(data.chartHumidity, lv_obj_get_width(parent) - 100);
	lv_obj_align(data.chartHumidity, parent, LV_ALIGN_IN_BOTTOM_RIGHT, -3, -5);
	lv_chart_init_points(data.chartHumidity, data.chartSerHumidity, 1);

	data.meterHumidity = lv_gauge_create(parent, NULL);
    static lv_style_t   styleMeter;
    lv_style_copy(&styleMeter, lv_obj_get_style(data.meterHumidity));
    styleMeter.body.main_color = LV_COLOR_MAKE(0xcc, 0xff, 0xff);
    styleMeter.body.grad_color = LV_COLOR_AQUA;
    styleMeter.body.radius = lv_obj_get_height(parent) / 4;
    lv_gauge_set_style(data.meterHumidity, &styleMeter);
    lv_gauge_set_critical_value(data.meterHumidity, 100);
    lv_gauge_set_scale(data.meterHumidity, 180, 31, 5);
    lv_gauge_set_range(data.meterHumidity, 0, 100);
    lv_gauge_set_needle_count(data.meterHumidity, 1, &LV_COLOR_BLACK);
    lv_gauge_set_value(data.meterHumidity, 0, 0);
    lv_obj_set_width(data.meterHumidity, (lv_obj_get_width(parent) - 100) * 0.80);
	lv_obj_align(data.meterHumidity, parent, LV_ALIGN_IN_TOP_MID, 50, 0);

	data.meterLblHumidity = lv_label_create(parent, NULL);
	lv_obj_align(data.meterLblHumidity, parent, LV_ALIGN_IN_TOP_LEFT, 102, 2);
	lv_label_set_text(data.meterLblHumidity, "0%");
}

static void createSensorsPressureView(void *tab)
{
    lv_obj_t	*parent = (lv_obj_t *)tab;
    lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
    lv_page_set_scrl_fit(parent, false, false);
    lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
    lv_page_set_scrl_height(parent, lv_obj_get_height(parent));
	lv_obj_set_height(parent, lv_obj_get_height(parent) - 40);
    static lv_style_t	style;
	lv_style_copy(&style, lv_obj_get_style(parent));
    style.body.padding.hor = -10;
    style.body.padding.ver = -10;
    style.body.padding.inner = 0;
	lv_obj_set_style(parent, &style);

    data.chartPressure = lv_chart_create(parent, NULL);
	data.chartSerPressure = lv_chart_add_series(data.chartPressure, LV_COLOR_RED);
	lv_chart_set_type(data.chartPressure, LV_CHART_TYPE_POINT);
	lv_chart_set_point_count(data.chartPressure, 30);
	lv_chart_set_range(data.chartPressure, 0, 100);
	lv_chart_set_div_line_count(data.chartPressure, 0, 0);
	lv_obj_set_width(data.chartPressure, lv_obj_get_width(parent) - 100);
    lv_obj_set_height(data.chartPressure, lv_obj_get_height(parent));
	lv_obj_align(data.chartPressure, parent, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
	lv_chart_init_points(data.chartPressure, data.chartSerPressure, 1);

    data.meterLblPressure = lv_label_create(parent, NULL);
    lv_label_set_text(data.meterLblPressure, "0hPa");
    lv_obj_align(data.meterLblPressure, data.chartPressure, LV_ALIGN_IN_TOP_LEFT, 2, 2);
}

static lv_res_t	changeTabview(SensorsPage_t index, lv_obj_t * obj)
{
    if (data.oldBtn != NULL)
        lv_btn_set_state(data.oldBtn, LV_BTN_STATE_REL);
	lv_tabview_set_tab_act(data.nav, index, true);
    lv_btn_set_state(obj, LV_BTN_STATE_PR);
    data.oldBtn = obj;
    return LV_RES_OK;
}

static lv_res_t	changeToTemperature(struct _lv_obj_t * obj)
{
    ESP_LOGI(TAG, "Change to temperature");
    return changeTabview(TEMPERATURE, obj);
}

static lv_res_t	changeToAll(struct _lv_obj_t * obj)
{
    ESP_LOGI(TAG, "Change to all");
    return changeTabview(ALL, obj);
}

static lv_res_t	changeToHumidity(struct _lv_obj_t * obj)
{
    ESP_LOGI(TAG, "Change to humidity");
    return changeTabview(HUMIDITY, obj);
}

static lv_res_t	changeToPressure(struct _lv_obj_t * obj)
{
    ESP_LOGI(TAG, "Change to pressure");
    return changeTabview(PRESSURE, obj);
}

static void    clickOnBtn(lv_obj_t *btn)
{
    if (btn != NULL){
        lv_action_t action = lv_btn_get_action(btn, LV_BTN_ACTION_CLICK);
        if (action != NULL)
            (*action)(btn);
        lv_list_set_btn_selected(data.navMenu, btn);
    }
}

SensorsPage_t getSensorsCurrentPage()
{
    return (SensorsPage_t)lv_list_get_btn_index(data.navMenu, data.oldBtn);
}

void    nextSensorsPage()
{
    if (xSemaphoreTake(lcdGetSemaphore(), pdMS_TO_TICKS(100)) == pdTRUE) {
        lv_obj_t *btn = lv_list_get_prev_btn(data.navMenu, data.oldBtn);
        
        clickOnBtn(btn);
        xSemaphoreGive(lcdGetSemaphore());
    }
}

void    previousSensorsPage()
{
    if (xSemaphoreTake(lcdGetSemaphore(), pdMS_TO_TICKS(100)) == pdTRUE) {
        lv_obj_t *btn = lv_list_get_next_btn(data.navMenu, data.oldBtn);
        
        clickOnBtn(btn);
        xSemaphoreGive(lcdGetSemaphore());
    }
}

void	createSensorsView(void *tab)
{
    lv_obj_t	*parent = (lv_obj_t *)tab;
    lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
    lv_page_set_scrl_fit(parent, false, true);
    lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
    lv_style_t	*style = lv_obj_get_style(parent);
    style->body.padding.hor = 0;
    style->body.padding.ver = 0;

    memset(&data, 0, sizeof(LcdSensors_t));
    data.temperature = &temperature;
    data.humidity = &humidity;
    data.pressure = &pressure;

    //Menu
    data.nav = lv_tabview_create(parent, NULL);    
    lv_tabview_set_btns_hidden(data.nav, true);                           /*Create a slider*/
    lv_style_t	*paddingOff = lv_obj_get_style(data.nav);
    paddingOff->body.padding.inner = 0;
    paddingOff->body.padding.ver = 0;
    paddingOff->body.padding.hor = 0;

    lv_obj_t *tab1 = lv_tabview_add_tab(data.nav, "All");
    lv_obj_t *tab2 = lv_tabview_add_tab(data.nav, "Temperature");
    lv_obj_t *tab3 = lv_tabview_add_tab(data.nav, "Humidity");
    lv_obj_t *tab4 = lv_tabview_add_tab(data.nav, "Pressure");

    data.navMenu = lv_list_create(parent, NULL);

    static lv_style_t style_btn_rel;
    lv_style_copy(&style_btn_rel, &lv_style_btn_rel);
    style_btn_rel.body.main_color = LV_COLOR_MAKE(0x30, 0x30, 0x30);
    style_btn_rel.body.grad_color = LV_COLOR_BLACK;
    style_btn_rel.body.border.color = LV_COLOR_SILVER;
    style_btn_rel.body.border.width = 1;
    style_btn_rel.body.border.opa = LV_OPA_50;
    style_btn_rel.body.radius = 0;
    style_btn_rel.body.padding.hor = 10;

    lv_obj_set_size(data.navMenu, 100, lv_obj_get_height(parent) + 5);
    lv_list_set_sb_mode(data.navMenu, LV_SB_MODE_AUTO);
    lv_obj_align(data.navMenu, parent, LV_ALIGN_IN_TOP_LEFT, 0, -5);

    lv_obj_t *tmp  = lv_list_add(data.navMenu, SYMBOL_LIST, "All", &changeToAll);
    lv_list_add(data.navMenu, NULL, "Temperature", &changeToTemperature);
    lv_list_add(data.navMenu, NULL, "Humidity", &changeToHumidity);
    lv_list_add(data.navMenu, NULL, "Pressure", &changeToPressure);
    
    lv_list_set_btn_selected(data.navMenu, tmp);
    lv_btn_set_state(tmp, LV_BTN_STATE_PR);
    data.oldBtn = tmp;

    lv_list_set_style(data.navMenu, LV_LIST_STYLE_BG, &lv_style_transp_tight);
    lv_list_set_style(data.navMenu, LV_LIST_STYLE_BTN_REL, &style_btn_rel);
    lv_list_set_sb_mode(data.navMenu, LV_SB_MODE_HIDE);

    createSensorsAllView(tab1);
	createSensorsTemperatureView(tab2);
	createSensorsHumidityView(tab3);
	createSensorsPressureView(tab4);
}

LcdSensors_t	*getSensorsData()
{
    return &data;
}