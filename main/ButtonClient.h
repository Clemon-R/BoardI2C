#ifndef BUTTONCLIENT_H_
#define BUTTONCLIENT_H_

#include "Main.h"

#include "lcd/Lcd.h"

typedef enum TypeClick_e
{
    Simple = 1,
    Double,
    Long,
    VeryLong
}			TypeClick;

void	setButtonCallback(void	(*callback)(uint32_t io_num, TypeClick type));

esp_err_t	startButtonClient();
esp_err_t	stopButtonClient();

#endif //!BUTTONCLIENT_H_