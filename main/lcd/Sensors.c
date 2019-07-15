#include "Sensors.h"

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
	lv_chart_set_range(data.chartTemperature, 0, 100);
	lv_chart_set_div_line_count(data.chartTemperature, 4, 8);
	lv_obj_set_width(data.chartTemperature, lv_obj_get_width(parent) - 100);
	lv_obj_align(data.chartTemperature, parent, LV_ALIGN_IN_BOTTOM_RIGHT, -3, -5);
	lv_chart_init_points(data.chartTemperature, data.chartSerTemperature, 1);

	data.meterTemperature = lv_lmeter_create(parent, NULL);
	lv_lmeter_set_scale(data.meterTemperature, 240, 31);
	lv_lmeter_set_range(data.meterTemperature, 0, 100);
	lv_lmeter_set_value(data.meterTemperature, 25);
	lv_obj_align(data.meterTemperature, parent, LV_ALIGN_IN_TOP_MID, 50, 10);

	data.meterLblTemperature = lv_label_create(data.meterTemperature, NULL);
	lv_obj_align(data.meterLblTemperature, data.meterTemperature, LV_ALIGN_CENTER, -12, 0);
	lv_label_set_text(data.meterLblTemperature, "25,00°C");
}

static lv_res_t	changeTabview(uint8_t index, lv_obj_t * obj)
{
    if (data.oldView != NULL)
        lv_btn_set_state(data.oldView, LV_BTN_STATE_REL);
	lv_tabview_set_tab_act(data.nav, index, true);
    lv_btn_set_state(obj, LV_BTN_STATE_PR);
    data.oldView = obj;
    return LV_RES_OK;
}

static lv_res_t	changeToTemperature(struct _lv_obj_t * obj)
{
    return changeTabview(1, obj);
}

static lv_res_t	changeToAll(struct _lv_obj_t * obj)
{
    return changeTabview(0, obj);
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

    lv_obj_t * tab1 = lv_tabview_add_tab(data.nav, "All");
    lv_obj_t * tab2 = lv_tabview_add_tab(data.nav, "Temperature");
    lv_obj_t * tab3 = lv_tabview_add_tab(data.nav, "Humidity");
    lv_obj_t * tab4 = lv_tabview_add_tab(data.nav, "Pressure");
    lv_obj_t * tab5 = lv_tabview_add_tab(data.nav, "Color");

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

    lv_obj_t *tmp  = lv_list_add(listMenu, SYMBOL_LIST, "All", &changeToAll);
    lv_list_add(listMenu, NULL, "Temperature", &changeToTemperature);
    //lv_list_add(listMenu, NULL, "Humidity", NULL);
    //lv_list_add(listMenu, NULL, "Pressure", NULL);
    //lv_list_add(listMenu, NULL, "Color", NULL);

    lv_list_set_style(listMenu, LV_LIST_STYLE_BG, &lv_style_transp_tight);
    lv_list_set_style(listMenu, LV_LIST_STYLE_BTN_REL, &style_btn_rel);
    lv_list_set_sb_mode(listMenu, LV_SB_MODE_HIDE);

    lv_btn_set_state(tmp, LV_BTN_STATE_PR);
    data.oldView = tmp;
    createSensorsAllView(tab1);
	createSensorsTemperatureView(tab2);
}

LcdSensors_t	*getSensorsData()
{
    return &data;
}