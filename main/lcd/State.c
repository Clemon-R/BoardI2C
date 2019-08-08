#include "State.h"
#include "../Main.h"

static LcdState_t	data;

void	createStatesView(void *tab)
{
    lv_obj_t	*parent = (lv_obj_t *)tab;
    lv_page_set_sb_mode(parent, LV_SB_MODE_HIDE);
    lv_page_set_scrl_fit(parent, false, true);
    lv_page_set_scrl_width(parent, lv_obj_get_width(parent));
    lv_style_t	*style = lv_obj_get_style(parent);
    style->body.padding.hor = 0;

	memset(&data, 0, sizeof(LcdState_t));

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

    data.ledWifi = lv_led_create(parent, NULL);
    lv_obj_set_style(data.ledWifi, &style_led);
    lv_obj_set_size(data.ledWifi, 20, 20);
    lv_obj_align(data.ledWifi, parent, LV_ALIGN_IN_TOP_MID, -15, lv_obj_get_height(parent) / 2 - 38);
    lv_led_set_bright(data.ledWifi, 0);

    //Mqtt

    lv_obj_t	*lblMqtt = lv_label_create(parent, NULL);
    lv_label_set_text(lblMqtt, SYMBOL_UPLOAD" Mqtt:");
    lv_obj_align(lblMqtt, parent, LV_ALIGN_IN_TOP_LEFT, 15, lv_obj_get_height(parent) / 2 - 13);

    data.ledMqtt = lv_led_create(parent, NULL);
    lv_obj_set_style(data.ledMqtt, &style_led);
    lv_obj_set_size(data.ledMqtt, 20, 20);
    lv_obj_align(data.ledMqtt, parent, LV_ALIGN_IN_TOP_MID, -15, lv_obj_get_height(parent) / 2 - 13);
    lv_led_set_bright(data.ledMqtt, 0);

    //Sensors

    lv_obj_t	*lblSensors = lv_label_create(parent, NULL);
    lv_label_set_text(lblSensors, SYMBOL_REFRESH" Sensors:");
    lv_obj_align(lblSensors, parent, LV_ALIGN_IN_TOP_LEFT, 15, lv_obj_get_height(parent) / 2 + 13);

    data.ledSensors = lv_led_create(parent, NULL);
    lv_obj_set_style(data.ledSensors, &style_led);
    lv_obj_set_size(data.ledSensors, 20, 20);
    lv_obj_align(data.ledSensors, parent, LV_ALIGN_IN_TOP_MID, -15, lv_obj_get_height(parent) / 2 + 13);
    lv_led_set_bright(data.ledSensors, 0);

    //Details states

    data.lblTemperature = lv_label_create(parent, NULL);
    lv_label_set_text(data.lblTemperature, "Temperature "SYMBOL_CLOSE);
    lv_obj_align(data.lblTemperature, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2 - 50);

    data.lblHumidity = lv_label_create(parent, NULL);
    lv_label_set_text(data.lblHumidity, "Humidity "SYMBOL_CLOSE);
    lv_obj_align(data.lblHumidity, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2 - 25);

    data.lblPressure = lv_label_create(parent, NULL);
    lv_label_set_text(data.lblPressure, "Pessure "SYMBOL_CLOSE);
    lv_obj_align(data.lblPressure, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2);

    data.lblColor = lv_label_create(parent, NULL);
    lv_label_set_text(data.lblColor, "Color "SYMBOL_CLOSE);
    lv_obj_align(data.lblColor, parent, LV_ALIGN_IN_TOP_LEFT, lv_obj_get_width(parent) / 2 + 3, lv_obj_get_height(parent) / 2 + 25);

    lv_obj_t    *version = lv_label_create(parent, NULL);
    char    buff[6];
    sprintf(buff, "V%s", getCurrentVersion());
    lv_label_set_text(version, buff);
    lv_obj_align(version, parent, LV_ALIGN_IN_BOTTOM_RIGHT, -2, 0);
}

LcdState_t	*getStateData()
{
	return &data;
}