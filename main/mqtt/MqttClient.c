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

static WifiState_t	_state = NONE;
static char	_running = false;

static const int WIFI_CONNECTED_BIT = BIT0;
static const int CONNECTED_BIT = BIT0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	char	*pos;
	char	tmp;
	MqttClientCommand_t	command;

	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			_state = INITIATIED;
			ESP_LOGI(TAG, "\033[4mClient is connected\033[0m");
			
			xEventGroupSetBits(_mqttEventGroup, CONNECTED_BIT);
			gpio_set_level(RGB_2, 1);

			esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/status", 0);
			ESP_LOGI(TAG, "Subscribed to /demo/rtone/esp32/status");
			esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/commands", 0);
			ESP_LOGI(TAG, "Subscribed to /demo/rtone/esp32/commands");
			esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/datas", 0);
			ESP_LOGI(TAG, "Subscribed to /demo/rtone/esp32/datas");
			break;
		case MQTT_EVENT_DISCONNECTED:
			_state = INITIATIED;
			ESP_LOGE(TAG, "\033[5mClient was disMqttconnected\033[0m");
			xEventGroupClearBits(_mqttEventGroup, CONNECTED_BIT);
			gpio_set_level(RGB_2, 0);
			break;
		case MQTT_EVENT_DATA:
			tmp = *event->data;
			*event->data = 0;
			pos = strrchr(event->topic, '/');
			if (pos == NULL)
				break;
			if (strcmp(pos + 1, "commands") == 0){	
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
	return ESP_OK;
}

static esp_mqtt_client_handle_t	initMqttClient(const char *url)
{
	esp_mqtt_client_config_t mqtt_cfg = {
		.uri = url,
		.event_handle = mqtt_event_handler,
		//.transport = MQTT_TRANSPORT_OVER_TCP
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
	return esp_mqtt_client_start(client);
}

static esp_err_t	stopMqtt(esp_mqtt_client_handle_t client)
{
	ESP_LOGI(TAG, "Stopping the mqtt...");
	if (!client)
		return ESP_FAIL;
	return esp_mqtt_client_stop(client);
}

char	isMqttConnected()
{
	return _state == INITIATIED && (xEventGroupWaitBits(_mqttEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) == CONNECTED_BIT;
}

static void	taskMqtt(void *arg)
{
	esp_mqtt_client_handle_t client = NULL;
	MqttConfig_t	*data = NULL;
	cJSON	*monitor = NULL;

    ESP_LOGI(TAG, "Initiating the task...");
	ESP_ERROR_CHECK(arg == NULL);
	data = (MqttConfig_t *)arg;

	_state = NONE;
	while (_running){
		if (!isMqttConnected()){
			if (_state <= DEINITIATIED && (xEventGroupWaitBits(_wifiEventGroup, WIFI_CONNECTED_BIT, false, true, pdMS_TO_TICKS(1000)) & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT){
				ESP_ERROR_CHECK((client = initMqttClient(data->url)) == NULL);
				ESP_ERROR_CHECK(startMqtt(client));
				_state = INITIATING;
			} else if(client && _state == INITIATIED && !isMqttConnected()) {
				ESP_ERROR_CHECK(stopMqtt(client));
				ESP_ERROR_CHECK(deinitMqttClient(client));
				_state = DEINITIATIED;
			}
		} else {
			if (xQueueReceive(_datas, (void *)&monitor, (TickType_t)pdMS_TO_TICKS(500)) == pdTRUE && monitor){
				char	*buff = cJSON_Print(monitor);

				if (!buff){
					cJSON_Delete(monitor);
					monitor = NULL;
					continue;
				}
				esp_mqtt_client_publish(client, "/demo/rtone/esp32/datas", buff, strlen(buff), 0, 0);
				cJSON_Delete(monitor);
				monitor = NULL;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	stopMqtt(client);
	deinitMqttClient(client);
	free(data);
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
		_datas = xQueueCreate(10, sizeof(cJSON *));
	_running = true;
    return xTaskCreate(taskMqtt, "mqttTask", 6000, tmp, tskIDLE_PRIORITY, &mqttTask);
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