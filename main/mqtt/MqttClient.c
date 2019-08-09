#include "MqttClient.h"
#include "../wifi/WifiClient.h"
#include "MqttClientCommandHandler.h"
#include "cJSON.h"

#include "../Main.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"

static const char *TAG = "\033[1;36mMqttClient\033[0m";
static TaskHandle_t	mqttTask = NULL;

static EventGroupHandle_t   _wifiEventGroup = NULL;
static EventGroupHandle_t   _mqttEventGroup = NULL;
static QueueHandle_t	_datas = NULL;
static QueueHandle_t    _handler = NULL;

static MqttConfig_t	    *_config = NULL;
static ClientState_t	_state = NONE;
static char	_running = false;
static char _restart = false;

static const int WIFI_CONNECTED_BIT = BIT0;
static const int CONNECTED_BIT = BIT0;

//Managing the state of the mqtt
static void refreshState(ClientState_t state)
{
    _state = state;
    switch (state)
    {
        case CONNECTED:
            gpio_set_level(RGB_2_RED, RGB_OFF);
            gpio_set_level(RGB_2_GREEN, RGB_ON);
            xEventGroupSetBits(_mqttEventGroup, CONNECTED_BIT);
            break;

        case DEINITIATIED:
        case DISCONNECTED:
            gpio_set_level(RGB_2_RED, RGB_ON);
            gpio_set_level(RGB_2_GREEN, RGB_OFF);
            xEventGroupClearBits(_mqttEventGroup, CONNECTED_BIT);
            break;    

        default:
            break;
    }
}

//Callback mqtt event
static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    BaseType_t  type = pdTRUE;

    xQueueSendFromISR(_handler, &event, &type);
    return ESP_OK;
}

//Mqtt event handler in the process
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
        //Too much channels on the server test mqtt, come with crashs
        esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/commands", 0); //Receiver
        esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/datas", 0); //Sender
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

//Init the client with the required data
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

//Destroying client and all allocated data
static esp_err_t	deinitMqttClient(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Deinitiating the mqtt...");
    if (!client)
        return ESP_FAIL;
    return esp_mqtt_client_destroy(client);
}

//Starting the connection
static esp_err_t	startMqtt(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Starting the mqtt...");
    if (!client)
        return ESP_FAIL;
    refreshState(CONNECTING);
    return esp_mqtt_client_start(client);
}

//Change only the state of the running process, to go to the end of the process
static esp_err_t	stopMqtt(esp_mqtt_client_handle_t client)
{
    ESP_LOGI(TAG, "Stopping the mqtt...");
    if (!client)
        return ESP_FAIL;
    refreshState(DISCONNECTING);
    return esp_mqtt_client_stop(client);
}

//The process
static void	taskMqtt(void *arg)
{
    esp_mqtt_client_handle_t client = NULL;
    esp_mqtt_event_handle_t event = NULL;
    char    *buff = NULL;
    cJSON	*monitor = NULL;

    ESP_LOGI(TAG, "Initiating the task...");
    gpio_set_level(RGB_2_RED, RGB_ON);
    ESP_ERROR_CHECK(arg == NULL);
    _config = (MqttConfig_t *)arg;

    refreshState(NONE);
    while (_running) {
        if (xQueueReceive(_handler, &event, 10) == pdTRUE)
            mqttClientHandler(event);
        if (_restart || _state != CONNECTED) {
            if (_state <= DEINITIATIED && (xEventGroupWaitBits(_wifiEventGroup, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY) & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT) {
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
            if (xQueueReceive(_datas, (void *)&monitor, 10) == pdTRUE && 
                monitor) {
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

//Convert enum to string
static char *getStringOfEnum(const AlertType_t type)
{
    switch (type)
    {
        case INFO:
        return "INFO";
        case MINOR:
        return "MINOR";
        case MAJOR:
        return "MAJOR";
    }
    return NULL;
}

/**
 * Public function
 **/

char	isMqttConnected()
{
    return _state == CONNECTED && (xEventGroupWaitBits(_mqttEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) == CONNECTED_BIT;
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
    if (!_handler)
        _handler = xQueueCreate(STACK_QUEUE, sizeof(esp_mqtt_event_handle_t));
    if (!_wifiEventGroup || !_mqttEventGroup || !_datas || !_handler) {
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

BaseType_t    createAlert(const char *description, const AlertType_t type, const char closed)
{
    cJSON   *json = cJSON_CreateObject();

    if (!json)
        return pdFALSE;
    cJSON   *jDescription = cJSON_CreateString((char *)description);
    cJSON   *jType = cJSON_CreateString(getStringOfEnum(type));
    cJSON   *jClosed = cJSON_CreateBool(closed);
    if (!jDescription || !jType || !jClosed){
        if (jDescription)
            cJSON_Delete(jDescription);
        if (jType)
            cJSON_Delete(jType);
        if (jClosed)
            cJSON_Delete(jClosed);

        return pdFALSE;
    }

    cJSON_AddStringToObject(json, "data", "alert");
    cJSON_AddItemToObject(json, "description", jDescription);
    cJSON_AddItemToObject(json, "type", jType);
    cJSON_AddItemToObject(json, "closed", jClosed);
    return xQueueSend(_datas, &json, 10);
}

BaseType_t    createStatus()
{
    cJSON   *json = cJSON_CreateObject();

    if (!json)
        return pdFALSE;

    cJSON_AddStringToObject(json, "data", "status");
    return xQueueSend(_datas, &json, 10);
}


esp_err_t   getSaveMqttConfig(MqttConfig_t *config)
{
    nvs_handle  my_handle;
    esp_err_t   ret;

    if (!config)
        return ESP_FAIL;
    ret = nvs_open(MQTT_STORAGE, NVS_READWRITE, &my_handle);
    if (ret == ESP_OK) {
        char    tmp[BUFF_SIZE];
        size_t  len = BUFF_SIZE;

        ret = nvs_get_str(my_handle, STORAGE_URL, tmp, &len);
        tmp[len] = 0;
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Changing mqtt url by the flash one...");
            free(config->url);
            config->url = (uint8_t *)strdup(tmp);
        }

        ret = nvs_get_i16(my_handle, STORAGE_PORT, &len);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Changing mqtt port by the flash one...");
            config->port = len;
        }

        nvs_close(my_handle);
        return ESP_OK;
    }
    return ESP_FAIL;
}


esp_err_t   saveMqttConfig(MqttConfig_t *config)
{
    nvs_handle  my_handle;
    esp_err_t   ret;

    if (!config)
        return ESP_FAIL;
    ret = nvs_open(MQTT_STORAGE, NVS_READWRITE, &my_handle);
    if (ret == ESP_OK) {
        ret = nvs_set_str(my_handle, STORAGE_URL, (char *)config->url);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Changing mqtt url in flash...");
        }

        ret = nvs_set_i16(my_handle, STORAGE_PORT, config->port);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Changing mqtt port in flash...");
        }

        nvs_close(my_handle);
        return ESP_OK;
    }
    return ESP_FAIL;
}