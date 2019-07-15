#include "Log.h"

static lv_obj_t	*_textarea = NULL;

void	createLogsView(void *tab)
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

    _textarea = textarea;
}

lv_obj_t	*getLogTa()
{
	return _textarea;
}