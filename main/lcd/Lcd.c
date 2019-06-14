#include "Lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tftspi.h"
#include "tft.h"
#include "spiffs_vfs.h"
#include "driver/spi_master.h"
#include "freertos/event_groups.h"

#include "../wifi/WifiClient.h"
#include "../mqtt/MqttClient.h"

static const char	*TAG = "\033[1;93mLcd\033[0m";
static TaskHandle_t	lcdTask = NULL;

static spi_lobo_device_handle_t	busSpi = NULL;
static char	_running = false;
static char	_buffer[BUFF_SIZE];

static const uint8_t	_pages = 3;
static const char	*_navbarMenus[] = {
	"Sensors",
	"Logs",
	"Settings"
};

static const uint8_t	_logSize = 17;
static char	**_logs = NULL;

static char	_currentIndex = -1;
static char	_nextIndex = -1;

static EventGroupHandle_t _eventGroup = NULL;
static const int REFRESH_LOG = BIT0;

static int	customVPrintF(const char *str, va_list arg)
{
	char	buffer[BUFF_SIZE];
	char	*last = NULL;
	uint32_t	index;

	vsprintf(_buffer, str, arg);
	if (_logs[_logSize - 1]){
		free(_logs[_logSize - 1]);
	}
	for (uint32_t i = _logSize - 1;i > 0;i--){
		_logs[i] = _logs[i - 1];
	}
	_logs[0] = strdup(_buffer);
	xEventGroupSetBits(_eventGroup, REFRESH_LOG);
	return vprintf(str, arg);
}

static esp_err_t	initTftLib()
{
	esp_err_t	ret;

	ESP_LOGI(TAG, "Initiating the LCD...");
	tft_disp_type = DISP_TYPE_ILI9341;
	_width = 240;  // smaller dimension
	_height = 320; // larger dimension1
	TFT_PinsInit();

	spi_lobo_device_handle_t spi;
	spi_lobo_bus_config_t buscfg={
		.miso_io_num=PIN_NUM_MISO,
		.mosi_io_num=PIN_NUM_MOSI,
		.sclk_io_num=PIN_NUM_CLK,
		.quadwp_io_num=-1,
		.quadhd_io_num=-1,
		.max_transfer_sz=6*320*240
	};
	spi_lobo_device_interface_config_t devcfg={
		.clock_speed_hz=10000000,           //Clock out at 10 MHz
		.mode=0,                                //SPI mode 0
		.spics_io_num=-1,                       // we will use external CS pin
		.spics_ext_io_num=PIN_NUM_CS,           // external CS pin
		.flags=LB_SPI_DEVICE_HALFDUPLEX, // ALWAYS SET  to HALF DUPLEX MODE!! for display spi
	};
	ret = spi_lobo_bus_add_device(TFT_HSPI_HOST, &buscfg, &devcfg, &spi);
	if (ret != ESP_OK)
		return ret;
	busSpi = spi;
	disp_spi = spi;

	// ==== Test select/deselect ====
	ret = spi_lobo_device_select(spi, 1);
	if (ret != ESP_OK)
		return ret;
	TFT_display_init();
	max_rdclock = find_rd_speed();

	spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);

	font_rotate = 0;
	text_wrap = 0;
	font_transparent = 1;
	font_forceFixed = 0;
	gray_scale = 0;
	TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
	TFT_setRotation(LANDSCAPE);
	TFT_setFont(COMIC10_FONT, NULL);
	TFT_resetclipwin();

	_logs = calloc(_logSize, sizeof(char *));
	if (!_logs)
		return ESP_FAIL;
	esp_log_set_vprintf(&customVPrintF);
	return ESP_OK;
}

static esp_err_t	deinitTftLib()
{
	esp_err_t	ret;

	ESP_LOGI(TAG, "Deinitiating the LCD...");
	TFT_display_deinit();
	ret = spi_lobo_bus_remove_device(busSpi);
	if (ret == ESP_OK){
		busSpi = NULL;
		esp_log_set_vprintf(&vprintf);
		for (int i = 0;_logs[i] && i < _logSize;i++)
			free(_logs[i]);
		free(_logs);
	}
	return ret;
}

static void	loadingScreen()
{
	ESP_LOGI(TAG, "Drawing loading screen...");
	TFT_fillRect(0,0,80,240,TFT_BLUE);
	TFT_fillRect(80,0,80,240,TFT_GREEN);
	TFT_fillRect(160,0,80,240,TFT_RED);
	TFT_fillRect(240,0,80,240,TFT_WHITE);

	_fg = TFT_WHITE;
	TFT_print("Loading program...", CENTER, CENTER);
}

static void	drawNavBar(uint8_t selected){
	uint32_t	navWidth = _height / 2;
	uint32_t	elemHeight = _height / 6;
	color_t		bgColor = { 31 << 3, 27 << 2, 0};
	color_t		bgColorSelected = { 12 << 3, 11 << 2, 0};

	_fg = TFT_WHITE;
	for (uint32_t i = 0;i < _pages;i++){
		TFT_fillRect(0, (elemHeight + 1) * i, navWidth, elemHeight, i == selected ? bgColorSelected : bgColor);
		font_transparent = 1;
		TFT_print(_navbarMenus[i], (navWidth - TFT_getStringWidth(_navbarMenus[i])) / 2, (elemHeight - TFT_getfontheight()) / 2 + (elemHeight + 1) * i);
		TFT_fillRect(0, elemHeight + (elemHeight + 1) * i, navWidth, 1, TFT_BLACK);
	}
	TFT_fillRect(navWidth, 0, 1, _height, TFT_BLACK);
	TFT_fillRect(0, (elemHeight + 1) * _pages, navWidth, _height - (elemHeight + 1) * _pages, TFT_WHITE);
}

static void	newPage(const char *title)
{
	uint32_t	navWidth = _height / 2;

	TFT_fillRect(navWidth + 1, 0, _width - navWidth - 1, _height, TFT_WHITE);
	_fg = TFT_BLACK;
	font_transparent = 1;
	ESP_LOGI(TAG, "Page: %s", title);
	TFT_print(title, navWidth + 11, 5);
	TFT_drawLine(navWidth + 6, 10 + TFT_getfontheight(), navWidth + 6 + TFT_getStringWidth(title) + 20, 10 + TFT_getfontheight(), _fg);
}

static void	printInContent(char *text, uint16_t x, uint16_t y, char all)
{
	char	*str = text;

	for (uint32_t i = 0;text && text[i];i++){
		if (text[i] == '\n'){
			snprintf(_buffer, i - (str - text) + 1, "%s", str);
			TFT_fillRect(x, y, all ? _width - x : TFT_getStringWidth(_buffer), TFT_getfontheight(), _bg);
			TFT_print(_buffer, x, y);
			y += TFT_getfontheight() + 2;
			str = text + i + 1;
		}
	}
	if (str && *str){
		TFT_fillRect(x, y, all ? _width - x : TFT_getStringWidth(str), TFT_getfontheight(), _bg);
		TFT_print(str, x, y);
	}
}

static void	refreshContent(uint8_t index)
{
	uint32_t	startX = _height / 2 + 3;
	uint32_t	startY = TFT_getfontheight() + 15;

	_bg = TFT_WHITE;
	_fg = TFT_BLACK;
	TFT_setFont(COMIC6_FONT, NULL);
	switch (index){
		case SENSORS:
		_bg = (color_t){8 << 3, 16 << 2, 8 << 3};
		if (_currentIndex != index){
			TFT_fillRect(startX, startY, _width - startX - 2, _height - startY - 2, _bg);
		}
		startX += 2;
		startY += 2;
		_fg = TFT_WHITE;
		printInContent("Test\nyes\nno", startX, startY, 0);
		break;

		case LOGS:
		if ((xEventGroupWaitBits(_eventGroup, REFRESH_LOG, false, false, pdMS_TO_TICKS(100)) & REFRESH_LOG) != REFRESH_LOG)
			break;
		xEventGroupClearBits(_eventGroup, REFRESH_LOG);
		for (uint32_t i = 0, last = 0;_logs && _logs[i] && i < _logSize;i++){
			if (i > 0){
				memcpy(_buffer + last, "\n", 1);
				last++;
			}
			char	*infos = strchr(_logs[i], ':');
			if (!infos)
				continue;
			infos += 2;
			char	*end = strchr(infos, '[');
			if (end){
				*end = 0;
			}
			uint16_t len = strlen(infos);
			memcpy(_buffer + last, infos, len);
			last += len;
			_buffer[last] = 0;
		}
		printInContent(_buffer, startX, startY, 1);
		break;

		case SETTINGS:
		sprintf(_buffer, "Wifi is %s\nMqtt is %s", isWifiConnected() ? "ON" : "OFF", isMqttConnected() ? "ON" : "OFF");
		printInContent(_buffer, startX, startY, 1);
		break;

		default:
		break;
	}
	TFT_setFont(COMIC10_FONT, NULL);
	_currentIndex = index;
}

static void	stateDot()
{
	TFT_fillCircle(_width - 20, 9, 8, TFT_BLUE);
}

static void	changePage(uint8_t index)
{
	index %= _pages;
	drawNavBar(index);
	newPage(_navbarMenus[index]);
	refreshContent(index);
	stateDot();
}

static color_t	getStateColor()
{
	color_t	tmp = TFT_RED;
	uint8_t	counter = 0;

	if (isWifiConnected())
		counter++;
	if (isMqttConnected())
		counter++;
	if (counter >= 3)
		tmp = TFT_GREEN;
	else if (counter != 0)
		tmp = TFT_ORANGE;
	return tmp;
}

static void taskLcd(void *args)
{
	color_t	stateColor = TFT_RED;

	ESP_LOGI(TAG, "Initiating the Task...");
	ESP_ERROR_CHECK(initTftLib());
	loadingScreen();
	vTaskDelay(pdMS_TO_TICKS(1000));
	_nextIndex = 0;
	while (_running){
		color_t	tmp = getStateColor();
		if (strcmp((char *)&tmp, (char *)&stateColor) != 0){
			TFT_fillCircle(_width - 20, 9, 8, tmp);
			stateColor = tmp;
		}
		if (_nextIndex >= 0 && _nextIndex < _pages){
			changePage(_nextIndex);
			_nextIndex = -1;
		} else {
			refreshContent(_currentIndex);
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	deinitTftLib();
	lcdTask = NULL;
	vTaskDelete(NULL);
}

/* Public function */

LcdPageIndex	getCurrentPage()
{
	return (LcdPageIndex)_currentIndex;
}

void	setNextPage(LcdPageIndex index)
{
	_nextIndex = index;
}

esp_err_t	startLcd()
{
	ESP_LOGI(TAG, "Starting the %s task...", TAG);
	if (_running)
		return ESP_FAIL;
	if (!_eventGroup)
        _eventGroup = xEventGroupCreate();
	_running = true;
	return xTaskCreate(&taskLcd, "TaskLcd", 4098, NULL, tskIDLE_PRIORITY, &lcdTask);
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