/**
 * @file lv_templ.h
 *
 */

#ifndef ILI9341_H
#define ILI9341_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl/lvgl.h"
#include "esp_err.h"

/*********************
 *      DEFINES
 *********************/

#define ILI9341_DC   (gpio_num_t)21
#define ILI9341_RST  (gpio_num_t)18
#define ILI9341_BCKL (gpio_num_t)5

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void ili9341_init(void);
void ili9341_deinit(void);
void ili9341_fill(int32_t x1, int32_t y1, int32_t x2, int32_t y2, lv_color_t color);
void ili9341_map(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t *color);
void ili9341_flush(int32_t x1, int32_t y1, int32_t x2, int32_t y2, const lv_color_t * color_map);

/**********************
 *      MACROS
 **********************/


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*ILI9341_H*/
