#include "WifiClient.h"

#include "../Main.h"
#include "driver/gpio.h"

static const char *TAG = "\033[1;35mWifiClient\033[0m";
static TaskHandle_t	wifiTask = NULL;
static char         _running = false;
static char         _restart = false;
static WifiConfig_t *_config = NULL;
static wifi_config_t    _wifi_config;
static QueueHandle_t    _handler = NULL;

static EventGroupHandle_t _wifiEventGroup = NULL;
const static int CONNECTED_BIT = BIT0;
static ClientState_t    _state = NONE;

static void refreshState(ClientState_t state)
{
    _state = state;
    switch (state)
    {
        case CONNECTED:
            xEventGroupSetBits(_wifiEventGroup, CONNECTED_BIT);
            gpio_set_level(RGB_1_RED, 1);
            gpio_set_level(RGB_1_GREEN, 0);
            break;
        case DISCONNECTED:
            xEventGroupClearBits(_wifiEventGroup, CONNECTED_BIT);
            gpio_set_level(RGB_1_GREEN, 1);
            gpio_set_level(RGB_1_RED, 0);
            break;

        default:
            break;
    }
}

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    xQueueSendFromISR(_handler, &event, pdTRUE);
    return ESP_OK;
}

static void wifiClientHandler(system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        if (_running && !_restart)
            esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        refreshState(CONNECTED);
        ESP_LOGI(TAG, "\033[4mWifi successfully connected\033[0m");
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if (isWifiConnected()) {
            ESP_LOGE(TAG, "\033[5mWifi was disconnected\033[0m");
            refreshState(DISCONNECTED);
        }
        if (_running && !_restart){
            ESP_LOGI(TAG, "Trying to connect again...");
            esp_wifi_connect();
        }
        break;
    default:
        break;
    }
}

static void     setConfig(const uint8_t *ssid, const uint8_t *password)
{
    ESP_LOGI(TAG, "Setting information for the connection...");
    ESP_LOGI(TAG, "Ssid: %s, password: %s", ssid, password);
    memcpy(_wifi_config.sta.ssid, ssid, strlen((char *)ssid) + 1); // +1 pour copier le \0
    memcpy(_wifi_config.sta.password, password, strlen((char *)password) + 1);
}

static esp_err_t    initWifiClient(const uint8_t *ssid, const uint8_t *password)
{
    esp_err_t   ret;

    refreshState(INITIATING);
    setConfig(ssid, password);

    ESP_LOGI(TAG, "Initiating stack TCPIP...");
    tcpip_adapter_init();

    ESP_LOGI(TAG, "Initiating Wifi...");
    esp_event_loop_init(wifi_event_handler, NULL);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK)
        return ret;
    ret = esp_wifi_set_storage(WIFI_STORAGE_RAM);
    if (ret != ESP_OK)
        return ret;
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK)
        return ret;
    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &_wifi_config);
    return ret;
}

static esp_err_t    deinitWifiClient()
{
    ESP_LOGI(TAG, "Deinitiating the wifi...");
    refreshState(DEINITIATING);
    return esp_wifi_deinit();
}

static esp_err_t    startWifi()
{
    ESP_LOGW(TAG, "Starting the wifi...");
    refreshState(CONNECTING);
    return esp_wifi_start();
}

static esp_err_t    stopWifi()
{
    ESP_LOGI(TAG, "Stopping the wifi...");
    refreshState(DISCONNECTING);
    if (isWifiConnected())
        esp_wifi_disconnect();
    return esp_wifi_stop();
}

static void    taskWifi(void *arg)
{
    WifiConfig_t   *data;
    system_event_t *event = NULL;

    ESP_LOGI(TAG, "Initiating the task...");
    gpio_set_level(RGB_1_RED, 0);
    ESP_ERROR_CHECK(arg == NULL);
    data = (WifiConfig_t *)arg;
    _config = data;
    refreshState(NONE);

    ESP_ERROR_CHECK(initWifiClient(data->ssid, data->password));
    refreshState(INITIATIED);
    ESP_ERROR_CHECK(startWifi());

    while (_running)
    {
        if (xQueueReceive(_handler, &event, 10) == pdTRUE)
            wifiClientHandler(event);
        if (_restart && _config){
            ESP_ERROR_CHECK(stopWifi());
            refreshState(DISCONNECTED);
            setConfig(_config->ssid, _config->password);
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &_wifi_config));
            _restart = false;
            ESP_ERROR_CHECK(startWifi());
        }
    }
    ESP_ERROR_CHECK(stopWifi());
    ESP_ERROR_CHECK(deinitWifiClient());
    refreshState(DEINITIATIED);
    free(_config);
    _config = NULL;
    wifiTask = NULL;
    gpio_set_level(RGB_1_RED, 1);
    vTaskDelete(NULL);
}

/**
 * Public function
 **/

char	isWifiConnected()
{
    return _state == CONNECTED && (xEventGroupWaitBits(_wifiEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) == CONNECTED_BIT;
}

esp_err_t    startWifiClient(WifiConfig_t   *config)
{
    WifiConfig_t    *tmp;

    ESP_LOGI(TAG, "Starting the %s task...", TAG);
    if (wifiTask)
        return ESP_FAIL;
    tmp = malloc(sizeof(WifiConfig_t));
    if (!tmp)
        return ESP_FAIL;
    memcpy(tmp, config, sizeof(WifiConfig_t));
    if (!_wifiEventGroup)
        _wifiEventGroup = xEventGroupCreate();
    if (!_handler)
        _handler = xQueueCreate(STACK_QUEUE, sizeof(system_event_t *));
    _running = true;
    return xTaskCreate(taskWifi, "wifiTask", 3072, tmp, tskIDLE_PRIORITY, &wifiTask);
}

esp_err_t   stopWifiClient()
{
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
    if (!wifiTask) {
        return ESP_FAIL;
    }
    _running = false;
    return ESP_OK;
}

EventGroupHandle_t  getWifiEventGroup()
{
    return _wifiEventGroup;
}

char	wifiIsUsed()
{
    return wifiTask != NULL;
}

esp_err_t    restartWifiClient(WifiConfig_t *config)
{
    WifiConfig_t    *tmp;

    if (_restart)
        return ESP_FAIL;
    if (config){ //New config
        tmp = malloc(sizeof(WifiConfig_t));
        if (!tmp)
            return ESP_FAIL;
        memcpy(tmp, config, sizeof(WifiConfig_t));
        if (_config){
            free(_config);
        }
        _config = tmp;
    }
    _restart = true;
    return ESP_OK;
}