#include "WifiClient.h"

static const char *TAG = "WifiClient";

static EventGroupHandle_t wifi_event_group = NULL;
static uint32_t thisBits = 0;

const static int CONNECTED_BIT = BIT0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "Wifi successfully connected");
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            if ((thisBits & CONNECTED_BIT) == CONNECTED_BIT){
                ESP_LOGE(TAG, "Wifi was disconnected");
                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            }
            ESP_LOGI(TAG, "Trying to connect again...");
            esp_wifi_connect();
            break;
        default:
            break;
    }
    return ESP_OK;
}

static int initWifi(wifi_client_t *data)
{
    int ret;
    wifi_config_t wifi_config;

    ESP_LOGI(TAG, "Setting the wifi...");
    memcpy((char *)wifi_config.sta.ssid, data->ssid, strlen(data->ssid) + 1); // +1 pour copier le \0
    memcpy((char *)wifi_config.sta.password, data->password, strlen(data->password) + 1);
    if (wifi_event_group == NULL)
        wifi_event_group = data->eventGroup;
    else{
        ESP_LOGI(TAG, "Already set, changing ssid and password...");
        if ((xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, false, portMAX_DELAY) & CONNECTED_BIT) == CONNECTED_BIT){
            esp_wifi_disconnect();
            thisBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
            if ((thisBits & CONNECTED_BIT) != CONNECTED_BIT)
                return ESP_FAIL;
        }
        wifi_event_group = data->eventGroup;
        ret = esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
        return ret;
    }

    tcpip_adapter_init();
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

void    taskWifi(void *arg)
{
    wifi_client_t   data;

    ESP_ERROR_CHECK(arg == NULL);
    data = *(wifi_client_t *)arg;
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "Launching Wifi Client for ssid: %s...", data.ssid);
    ESP_ERROR_CHECK(initWifi(&data));
    ESP_LOGI(TAG, "Starting the wifi on:[%s]", data.ssid);
    ESP_ERROR_CHECK(esp_wifi_start());
    while (1){
        vTaskDelay(portMAX_DELAY);
    }
    esp_wifi_stop();
    wifi_event_group = NULL;
	vTaskDelete(NULL);
}
