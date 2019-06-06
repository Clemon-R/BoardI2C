#include "MqttClient.h"

static const char *TAG = "MqttClient";
static EventGroupHandle_t   mqttEventGroup;

static const int CONNECTED_BIT = BIT1;

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
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
            ESP_LOGI(TAG, "Error happend");
            break;
    }
    return ESP_OK;
}

esp_err_t    launchMqtt(EventGroupHandle_t eventGroup, const char *url)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = url,
        .event_handle = mqtt_event_handler,
        .transport = MQTT_TRANSPORT_OVER_TCP
        // .user_context = (void *)your_context
    };
    mqttEventGroup = eventGroup;

    ESP_LOGI(TAG, "Launching Mqtt Client...");
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    return esp_mqtt_client_start(client);
}