#include "Ota.h"
#include "freertos/task.h"
#include "WifiClient.h"
#include "../Main.h"
#include "../lcd/Lcd.h"
#include "../mqtt/MqttClient.h"

static const char *TAG = "Ota";

static char	_running = false;
static uint32_t   _binLen = 0;
static uint32_t   _binWritten = 0;

//Http handler
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s, diff=%d", evt->header_key, evt->header_value, strcmp(evt->header_key, "Content-Length"));
            if (strcmp(evt->header_key, "Content-Length") == 0){
                ESP_LOGI(TAG, "Size saved");
                _binLen = atoi(evt->header_value);
            }
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (_binLen > 0){
                updateToNewFirmware((_binWritten * 2 + evt->data_len) / 2 * 100 / _binLen, NULL, true);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            updateToNewFirmware(50, NULL, true);
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

//Steps of updating
static esp_err_t	launchOta(const esp_http_client_config_t *config, const char *targetVersion)
{
    if (!config) {
        ESP_LOGE(TAG, "esp_http_client config not found");
        return ESP_ERR_INVALID_ARG;
    }

    if (!config->cert_pem && !config->use_global_ca_store) {
        ESP_LOGE(TAG, "Server certificate not found, either through configuration or global CA store");
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_handle_t client = esp_http_client_init(config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialise HTTP connection");
        return ESP_FAIL;
    }

    /*if (esp_http_client_get_transport_type(client) != HTTP_TRANSPORT_OVER_SSL) {
        ESP_LOGE(TAG, "Transport is not over HTTPS");
        return ESP_FAIL;
    }*/

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        return err;
    }
    esp_http_client_fetch_headers(client);

    esp_ota_handle_t update_handle = 0;
    const esp_partition_t *update_partition = NULL;
    ESP_LOGI(TAG, "Starting OTA...");
    updateToNewFirmware(0, targetVersion, true); //Popup
    update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "Passive OTA partition not found");
        http_cleanup(client);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed, error=%d", err);
        http_cleanup(client);
        return err;
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
    ESP_LOGI(TAG, "Please Wait. This may take time");

    esp_err_t ota_write_err = ESP_OK;
    char *upgrade_data_buf = (char *)malloc(OTA_BUF_SIZE);
    if (!upgrade_data_buf) {
        ESP_LOGE(TAG, "Couldn't allocate memory to upgrade data buffer");
        return ESP_ERR_NO_MEM;
    }
    while (1) {
        int data_read = esp_http_client_read(client, upgrade_data_buf, OTA_BUF_SIZE);
        if (data_read == 0) {
            ESP_LOGI(TAG, "Connection closed,all data received");
            break;
        }
        if (data_read < 0) {
            ESP_LOGE(TAG, "Error: SSL data read error");
            break;
        }
        if (data_read > 0) {
            ota_write_err = esp_ota_write( update_handle, (const void *)upgrade_data_buf, data_read);
            if (ota_write_err != ESP_OK) {
                break;
            }
            _binWritten += data_read;
            updateToNewFirmware(_binWritten * 100 / _binLen, NULL, true);
            ESP_LOGD(TAG, "Written image length %d", _binWritten);
        }
    }
    free(upgrade_data_buf);
    http_cleanup(client); 
    ESP_LOGI(TAG, "Total binary data length writen: %d", _binWritten);
    
    esp_err_t ota_end_err = esp_ota_end(update_handle);
    if (ota_write_err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_write failed! err=0x%d", err);
        return ota_write_err;
    } else if (ota_end_err != ESP_OK) {
        ESP_LOGE(TAG, "Error: esp_ota_end failed! err=0x%d. Image is invalid", ota_end_err);
        return ota_end_err;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed! err=0x%d", err);
        return err;
    }
    ESP_LOGI(TAG, "esp_ota_set_boot_partition succeeded"); 

    return ESP_OK;
}

//Process
static void	taskOta(void * pvParameter)
{
    char    buff[BUFF_SIZE];
    char    *version = (char *)pvParameter;
    esp_http_client_config_t config = {
        .url = NULL,
        .cert_pem = "",
        .event_handler = _http_event_handler,
    };

    if (version && isWifiConnected()){
        sprintf(buff, BIN_URL, version);
        config.url = buff;
        ESP_LOGI(TAG, "Starting OTA ...");
        esp_err_t ret = launchOta(&config, version);
        if (ret == ESP_OK) {
            stopLcd();
            while (lcdIsWorking())
                vTaskDelay(pdMS_TO_TICKS(100));
            esp_restart();
        } else {
            ESP_LOGE(TAG, "Firmware Upgrades Failed");
            updateToNewFirmware(0, NULL, false);
            createAlert("update_firmware", "Failed to update the firmware", MAJOR, true);
        }
        free(version);
    }
	_running = false;
	vTaskDelete(NULL);
}

//Launching task to download and do the change of firmware
esp_err_t	launchUpdate(char *version)
{
    char *tmp = NULL;

	if (_running || !version)
		return ESP_FAIL;
	tmp = strdup(version);
    if (!tmp)
        return ESP_FAIL;
    _running = true;
	return xTaskCreate(taskOta, "Task OTA", 4096, tmp, tskIDLE_PRIORITY, NULL);
}