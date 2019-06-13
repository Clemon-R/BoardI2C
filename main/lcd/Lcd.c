#include "Lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tftspi.h"
#include "tft.h"
#include "spiffs_vfs.h"
#include "driver/spi_master.h"

static const char	*TAG = "\033[1;93mLcd\033[0m";

static spi_lobo_device_handle_t	busSpi = NULL;
static TaskHandle_t	lcdTask = NULL;
static char	_buffer[BUFF_SIZE];

static const uint8_t	_pages = 3;
static const char	*_navbarMenus[] = {
	"Sensors",
	"Logs",
	"Settings"
};

static char	_currentIndex = -1;

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
	TFT_setFont(DEF_SMALL_FONT, NULL);
	TFT_resetclipwin();
	return ESP_OK;
}

static esp_err_t	deinitTftLib()
{
	esp_err_t	ret;

	ESP_LOGI(TAG, "Deinitiating the LCD...");
	ret = spi_lobo_bus_remove_device(busSpi);
	if (ret == ESP_OK){
		busSpi = NULL;
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

	selected %= _pages;
	_fg = TFT_WHITE;
	font_transparent = 1;
	for (uint32_t i = 0;i < _pages;i++){
		TFT_fillRect(0, (elemHeight + 1) * i, navWidth, elemHeight, i == selected ? bgColorSelected : bgColor);
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

static void	printInContent(char *text, uint16_t x, uint16_t y)
{
	char	*str = text;

	for (uint32_t i = 0;text && text[i];i++){
		if (text[i] == '\n'){
			snprintf(_buffer, i - (str - text) + 1, "%s", str);
			TFT_fillRect(x, y,TFT_getStringWidth(_buffer), TFT_getfontheight(), _bg);
			TFT_print(_buffer, x, y);
			y += TFT_getfontheight() + 2;
			str = text + i + 1;
		}
	}
	if (str && *str){
		TFT_fillRect(x, y, TFT_getStringWidth(str), TFT_getfontheight(), _bg);
		TFT_print(str, x, y);
	}
}

static void	refreshContent(uint8_t index)
{
	uint32_t	startX = _height / 2 + 3;
	uint32_t	startY = TFT_getfontheight() + 15;

	font_transparent = 0;
	_bg = TFT_WHITE;
	_fg = TFT_BLACK;
	switch (index){
		case SENSORS:
		_bg = (color_t){8 << 3, 16 << 2, 8 << 3};
		if (_currentIndex != index){
			TFT_fillRect(startX, startY, _width - startX - 2, _height - startY - 2, _bg);
		}
		startX += 2;
		startY += 2;
		_fg = TFT_WHITE;
		printInContent("Test\nyes\nno", startX, startY);
		break;

		case LOGS:
		break;

		case SETTINGS:
		break;

		default:
		break;
	}
	_currentIndex = index;
}

static void	clearContent()
{
	uint32_t	startX = _height / 2 + 3;
	uint32_t	startY = TFT_getfontheight() + 15;

	TFT_fillRect(startX, startY, _width - startX - 1, _height - startY - 1, TFT_WHITE);
}

void	changePage(uint8_t index)
{
	drawNavBar(index);
	newPage(_navbarMenus[index]);
	clearContent();
	refreshContent(index);
}

static void taskLcd(void *args)
{
	ESP_LOGI(TAG, "Initiating the Task...");
	ESP_ERROR_CHECK(initTftLib());
	loadingScreen();
	vTaskDelay(pdMS_TO_TICKS(1000));
	changePage(0);
	while (1){
		refreshContent(0);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
	deinitTftLib();
	lcdTask = NULL;
	vTaskDelete(NULL);
}

esp_err_t	startLcd()
{
	ESP_LOGI(TAG, "Starting the %s task...", TAG);
	if (lcdTask)
		return ESP_FAIL;
	return xTaskCreate(&taskLcd, "TaskLcd", 4098, NULL, tskIDLE_PRIORITY, &lcdTask);
}

esp_err_t	stopLcd()
{
	ESP_LOGI(TAG, "Stopping the %s task...", TAG);
	if (!lcdTask)
		return ESP_FAIL;
	vTaskResume(lcdTask);
	return ESP_OK;
}