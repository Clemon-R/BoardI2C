#include "MqttClient.h"

static const char *TAG = "MqttClient";
static EventGroupHandle_t   wifiEventGroup;
static EventGroupHandle_t   mqttEventGroup;

static const int WIFI_CONNECTED_BIT = BIT0;
static const int CONNECTED_BIT = BIT0;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
	esp_mqtt_client_handle_t client = event->client;
	//int msg_id;
	// your_context_t *context = event->context;
	switch (event->event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "Connected");
			xEventGroupSetBits(mqttEventGroup, CONNECTED_BIT);
			//msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);
			//ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

			esp_mqtt_client_subscribe(client, "/demo/rtone/esp32/status", 0);
			ESP_LOGI(TAG, "Subscribed to /demo/rtone/esp32/status");
			break;
		case MQTT_EVENT_DISCONNECTED:
			ESP_LOGI(TAG, "Disconnected");
			xEventGroupClearBits(mqttEventGroup, CONNECTED_BIT);
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

void	taskMqtt(void *arg)
{
	esp_mqtt_client_handle_t client = NULL;

	ESP_ERROR_CHECK(arg == NULL);
	mqtt_client_t	data = *(mqtt_client_t *)arg;
	esp_mqtt_client_config_t mqtt_cfg = {
		.uri = data.url,
		.event_handle = mqtt_event_handler,
		//.transport = MQTT_TRANSPORT_OVER_TCP
	};

	wifiEventGroup = data.eventGroup;
	mqttEventGroup = xEventGroupCreate();
	while (1){
		if ((xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY) & WIFI_CONNECTED_BIT) == WIFI_CONNECTED_BIT
		&& (xEventGroupWaitBits(mqttEventGroup, CONNECTED_BIT, false, false, 0) & CONNECTED_BIT) != CONNECTED_BIT){
			if (client != NULL){
				ESP_LOGI(TAG, "Destroying old instance...");
				esp_mqtt_client_destroy(client);
			}
			ESP_LOGI(TAG, "Launching Mqtt Client...");
			ESP_LOGI(TAG, "Initiating Mqtt Client for url %s ...", data.url);
			client = esp_mqtt_client_init(&mqtt_cfg);
			ESP_ERROR_CHECK(client == NULL);
			ESP_ERROR_CHECK(esp_mqtt_client_start(client));
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	if (client != NULL)
		esp_mqtt_client_destroy(client);
	mqttEventGroup = NULL;
	vTaskDelete(NULL);
}