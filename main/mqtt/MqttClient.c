#include "MqttClient.h"
#include "../wifi/WifiClient.h"

static const char *TAG = "MqttClient";
static TaskHandle_t	mqttTask = NULL;

static EventGroupHandle_t   _wifiEventGroup = NULL;
static EventGroupHandle_t   _mqttEventGroup = NULL;

static WifiState_t	_state = NONE;
static char	_running = false;

static const int WIFI_CONNECTED_BIT = BIT0;
static const int CONNECTED_BIT = BIT0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	//int msg_id;
	// your_context_t *context = event->context;

	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			_state = INITIATIED;
			ESP_LOGI(TAG, "Connected");
			xEventGroupSetBits(_mqttEventGroup, CONNECTED_BIT);
			//msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
			//ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/status", 0);
			ESP_LOGI(TAG, "Subscribed to /demo/rtone/esp32/status");
			break;
		case MQTT_EVENT_DISCONNECTED:
			_state = INITIATIED;
			if ((xEventGroupWaitBits(_mqttEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) != CONNECTED_BIT){
				ESP_LOGI(TAG, "Disconnected");
				xEventGroupClearBits(_mqttEventGroup, CONNECTED_BIT);
			}
			break;
		case MQTT_EVENT_PUBLISHED:
			ESP_LOGI(TAG, "Data successfully published - msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA:
			ESP_LOGI(TAG, "Data received");
			printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			printf("DATA=%.*s\r\n", event->data_len, event->data);
			break;
		case MQTT_EVENT_ERROR:
			ESP_LOGE(TAG, "Error happend");
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

static char	isConnected()
{
	return (xEventGroupWaitBits(_mqttEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) == CONNECTED_BIT;
}

static void	taskMqtt(void *arg)
{
	esp_mqtt_client_handle_t client = NULL;
	MqttConfig_t	*data = NULL;

    ESP_LOGI(TAG, "Initiating the task...");
	ESP_ERROR_CHECK(arg == NULL);
	data = (MqttConfig_t *)arg;

	_state = NONE;
	while (_running){
		if (!isConnected()){
			if (_state <= DEINITIATIED && (xEventGroupWaitBits(_wifiEventGroup, WIFI_CONNECTED_BIT, false, true, pdMS_TO_TICKS(1000)) & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT){
				ESP_ERROR_CHECK((client = initMqttClient(data->url)) == NULL);
				ESP_ERROR_CHECK(startMqtt(client));
				_state = INITIATING;
			} else if(client && _state == INITIATIED && !isConnected()) {
				ESP_ERROR_CHECK(stopMqtt(client));
				ESP_ERROR_CHECK(deinitMqttClient(client));
				_state = DEINITIATIED;
			}
		}
	}
	stopMqtt(client);
	deinitMqttClient(client);
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
	_running = true;
    return xTaskCreate(taskMqtt, "mqttTask", 4098, tmp, tskIDLE_PRIORITY, &mqttTask);
}

esp_err_t	stopMqttClient()
{
    ESP_LOGI(TAG, "Stopping the %s task...", TAG);
	if (!_running)
		return ESP_FAIL;
	_running = false;
	return ESP_OK;
}