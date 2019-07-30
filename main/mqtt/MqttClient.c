#include "MqttClient.h"
#include "../wifi/WifiClient.h"
#include "MqttClientCommandHandler.h"
#include "cJSON.h"

#include "../Main.h"
#include "driver/gpio.h"

static const char *TAG = "\033[1;36mMqttClient\033[0m";
static TaskHandle_t	mqttTask = NULL;

static EventGroupHandle_t   _wifiEventGroup = NULL;
static EventGroupHandle_t   _mqttEventGroup = NULL;
static QueueHandle_t	_datas = NULL;
static QueueHandle_t	_alerts = NULL;
static QueueHandle_t    _handler = NULL;

static MqttConfig_t	    *_config = NULL;
static ClientState_t	_state = NONE;
static char	_running = false;
static char _restart = false;

static const int WIFI_CONNECTED_BIT = BIT0;
static const int CONNECTED_BIT = BIT0;

static void refreshState(ClientState_t state)
{
    _state = state;
    switch (state)
    {
        case CONNECTED:
            gpio_set_level(RGB_2_RED, 1);
            gpio_set_level(RGB_2_GREEN, 0);
            xEventGroupSetBits(_mqttEventGroup, CONNECTED_BIT);
            break;

        case DEINITIATIED:
        case DISCONNECTED:
            gpio_set_level(RGB_2_RED, 0);
            gpio_set_level(RGB_2_GREEN, 1);
            xEventGroupClearBits(_mqttEventGroup, CONNECTED_BIT);
            break;    

        default:
            break;
    }
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    xQueueSendFromISR(_handler, &event, pdTRUE);
    return ESP_OK;
}

static void mqttClientHandler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    char	*pos;
    char	tmp;
    MqttClientCommand_t	command;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "\033[4mClient is connected\033[0m");
        refreshState(CONNECTED);

        ESP_LOGI(TAG, "Subscribing to all the required channels...");
        esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/status", 0);
        esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/commands", 0);
        esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/datas", 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        if (_state == CONNECTED){
            refreshState(DISCONNECTED);
            ESP_LOGE(TAG, "\033[5mClient was disconnected\033[0m");
        }
        break;
    case MQTT_EVENT_DATA:
        tmp = *event->data;
        *event->data = 0;
        pos = strrchr(event->topic, '/');
        if (pos == NULL)
            break;
        if (strcmp(pos + 1, "commands") == 0) {
            *event->data = tmp;
            ESP_LOGI(TAG, "Command received");
            event->data[event->data_len] = 0;

            cJSON	*monitor = cJSON_Parse(event->data);
            if (!monitor)
                break;
            cJSON	*type = cJSON_GetObjectItem(monitor, "type");
            if (!monitor || !cJSON_IsNumber(type))
                break;
            command.type = (MqttClientCommandType)type->valueint;
            command.data = monitor;
            launchCommand(&command);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "\033[5mError happend\033[0m");
        break;
    default:
        break;
    }
}

static esp_mqtt_client_handle_t	initMqttClient(const uint8_t *url, const uint16_t port)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = (char *)url,
        .event_handle = mqtt_event_handler,
        .port = port, //for mqtt default port is 1883
        .transport = MQTT_TRANSPORT_OVER_TCP //Using protocol tcp to connect
    };

    ESP_LOGI(TAG, "Initiating the mqtt...");
    return esp_mqtt_client_init(&mqtt_cfg);
}

static esp_err_t	deinitMqttClient(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Deinitiating the mqtt...");
    if (!client)
        return ESP_FAIL;
    return esp_mqtt_client_destroy(client);
}

static esp_err_t	startMqtt(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Starting the mqtt...");
    if (!client)
        return ESP_FAIL;
    refreshState(CONNECTING);
    return esp_mqtt_client_start(client);
}

static esp_err_t	stopMqtt(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Stopping the mqtt...");
    if (!client)
        return ESP_FAIL;
    refreshState(DISCONNECTING);
    return esp_mqtt_client_stop(client);
}

/**
 * Public function
 **/

char	isMqttConnected()
{
    return _state == CONNECTED && (xEventGroupWaitBits(_mqttEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) == CONNECTED_BIT;
}

static void	taskMqtt(void *arg)
{
    esp_mqtt_client_handle_t client = NULL;
    esp_mqtt_event_handle_t event = NULL;
    char    *buff = NULL;
    cJSON	*monitor = NULL;

    ESP_LOGI(TAG, "Initiating the task...");
    gpio_set_level(RGB_2_RED, 0);
    ESP_ERROR_CHECK(arg == NULL);
    _config = (MqttConfig_t *)arg;

    refreshState(NONE);
    while (_running) {
        if (xQueueReceive(_handler, &event, 10) == pdTRUE)
            mqttClientHandler(event);
        if (_restart || _state != CONNECTED) {
            if (_state <= DEINITIATIED && (xEventGroupWaitBits(_wifiEventGroup, WIFI_CONNECTED_BIT, false, true, 10) & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT) {
                refreshState(INITIATING);
                ESP_ERROR_CHECK((client = initMqttClient(_config->url, _config->port)) == NULL);
                ESP_ERROR_CHECK(startMqtt(client));
                refreshState(INITIATIED);
            } else if(_restart || (client && _state == DISCONNECTED)) {
                refreshState(DEINITIATING);
                ESP_ERROR_CHECK(stopMqtt(client) && !_restart);
                ESP_ERROR_CHECK(deinitMqttClient(client) && !_restart);
                refreshState(DEINITIATIED);
            }
            _restart = false;
        } else if (_state == CONNECTED) {
            if (xQueueReceive(_datas, (void *)&monitor, 10) == pdTRUE && monitor) {
                buff = cJSON_Print(monitor);

                if (buff) {
                    esp_mqtt_client_publish(client, "/demo/rtone/esp32/datas", buff, strlen(buff), 0, 0);
                }
                cJSON_Delete(monitor);
                free(buff);
                monitor = NULL;
            }
        }
        vTaskDelay(30);
    }
    stopMqtt(client);
    deinitMqttClient(client);
    free(_config);
    mqttTask = NULL;
    vTaskDelete(NULL);
}

esp_err_t	startMqttClient(MqttConfig_t *config)
{
    MqttConfig_t    *tmp;

    ESP_LOGI(TAG, "Starting the %s task...", TAG);
    if (_running)
        return ESP_FAIL;
    tmp = malloc(sizeof(MqttConfig_t));
    if (!tmp)
        return ESP_FAIL;
    memcpy(tmp, config, sizeof(MqttConfig_t));
    if (!_mqttEventGroup)
        _mqttEventGroup = xEventGroupCreate();
    if (!_wifiEventGroup)
        _wifiEventGroup = getWifiEventGroup();
    if (!_datas)
        _datas = xQueueCreate(STACK_QUEUE, sizeof(cJSON *));
    if (!_alerts)
        _alerts = xQueueCreate(STACK_QUEUE, sizeof(cJSON *));
    if (!_handler)
        _handler = xQueueCreate(STACK_QUEUE, sizeof(esp_mqtt_event_handle_t));
    if (!_wifiEventGroup || !_mqttEventGroup || !_datas || !_alerts || !_handler) {
        ESP_LOGE(TAG, "Missing some parameters");
        return ESP_FAIL;
    }
    _running = true;
    return xTaskCreate(taskMqtt, "mqttTask", 4096, tmp, tskIDLE_PRIORITY, &mqttTask);
}

esp_err_t	stopMqttClient()
{
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
    if (!_running)
        return ESP_FAIL;
    _running = false;
    return ESP_OK;
}

QueueHandle_t	getQueueDatas()
{
    return _datas;
}

char    mqttIsRunning()
{
    return _running;
}

void    restartMqttClient(MqttConfig_t *config)
{
    _config = config;
    _restart = true;
}