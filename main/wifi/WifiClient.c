#include "WifiClient.h"

static const char *TAG = "WifiClient";
static TaskHandle_t	wifiTask = NULL;

static EventGroupHandle_t wifiEventGroup = NULL;
const static int CONNECTED_BIT = BIT0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Wifi successfully connected");
        xEventGroupSetBits(wifiEventGroup, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        if ((xEventGroupWaitBits(wifiEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) == CONNECTED_BIT) {
            ESP_LOGE(TAG, "Wifi was disconnected");
            xEventGroupClearBits(wifiEventGroup, CONNECTED_BIT);
        }
        ESP_LOGI(TAG, "Trying to connect again...");
        esp_wifi_connect();
        break;
    default:
        break;
    }
    return ESP_OK;
}

static esp_err_t    initWifiClient(const char *ssid, const char *password)
{
    wifi_config_t wifi_config;
    esp_err_t   ret;

    ESP_LOGI(TAG, "Setting information for the connection...");
    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid) + 1); // +1 pour copier le \0
    memcpy(wifi_config.sta.password, password, strlen(password) + 1);

    ESP_LOGI(TAG, "Initiating stack TCPIP...");
    tcpip_adapter_init();

    ESP_LOGI(TAG, "Initiating Wifi...");
    ret = esp_event_loop_init(wifi_event_handler, NULL);
    if (ret != ESP_OK)
        return ret;
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
    ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    return ret;
}

static esp_err_t    deinitWifiClient()
{
    ESP_LOGI(TAG, "Deinitiating the wifi...");
    return esp_wifi_deinit();
}

static esp_err_t    startWifi()
{
    ESP_LOGW(TAG, "Starting the wifi...");
    return esp_wifi_start();
}

static esp_err_t    stopWifi()
{
    ESP_LOGI(TAG, "Stopping the wifi...");
    return esp_wifi_start();
}

static void    taskWifi(void *arg)
{
    WifiConfig_t   *data;

    ESP_LOGI(TAG, "Initiating the task...");
    ESP_ERROR_CHECK(arg == NULL);
    data = (WifiConfig_t *)arg;

    ESP_ERROR_CHECK(initWifiClient(data->ssid, data->password));
    ESP_ERROR_CHECK(startWifi());

    vTaskSuspend(NULL);

    ESP_ERROR_CHECK(stopWifi());
    ESP_ERROR_CHECK(deinitWifiClient());
    free(data);
    vTaskDelete(NULL);
}

esp_err_t    startWifiClient(WifiConfig_t   *config)
{
    WifiConfig_t    *tmp;

    ESP_LOGI(TAG, "Starting the %s task...", TAG);
    tmp = malloc(sizeof(WifiConfig_t));
    if (!tmp)
        return ESP_FAIL;
    memcpy(tmp, config, sizeof(WifiConfig_t));
    if (!wifiEventGroup)
        wifiEventGroup = xEventGroupCreate();
    return xTaskCreate(taskWifi, "wifiTask", 4098, tmp, tskIDLE_PRIORITY, &wifiTask);
}

esp_err_t   stopWifiClient()
{
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
    if (!wifiTask){
        return ESP_FAIL;
    }
    vTaskResume(wifiTask);
    return ESP_OK;
}

EventGroupHandle_t  getWifiEventGroup()
{
    return wifiEventGroup;
}