#include "MqttClientCommandHandler.h"

#include "driver/gpio.h"
#include "cJSON.h"

#include "../Main.h"

#include "../wifi/WifiClient.h"
#include "../wifi/Ota.h"
#include "../sensors/SensorClient.h"
#include "../lcd/Lcd.h"

static const char	*TAG = "\033[1;36mMqttClientCommandHandler\033[0m";

static void	taskCommandHandler(void *args)
{
    MqttClientCommand_t	*command = (MqttClientCommand_t *)args;
    cJSON	*param;

    ESP_LOGI(TAG, "Starting the task for the command...");
    if (command) {
        switch (command->type) {
        case SENSOR:
            param = cJSON_GetObjectItem(command->data, "state");
            if (param && cJSON_IsBool(param)) {
                if (cJSON_IsTrue(param)) {
                    startSensorClient();
                } else {
                    stopSensorClient();
                }
            }
            param = cJSON_GetObjectItem(command->data, "delai");
            if (param && cJSON_IsNumber(param)) {
                setRefreshDelai(pdMS_TO_TICKS(param->valueint));
            }
            break;

        case REBOOT:
            esp_restart();
            break;

        case LCD:
            param = cJSON_GetObjectItem(command->data, "state");
            if (param && cJSON_IsBool(param)) {
                if (cJSON_IsTrue(param)) {
                    startLcd();
                } else {
                    stopLcd();
                }
            }
            break;
        case WIFI:
            param = cJSON_GetObjectItem(command->data, "restart");
            if (param && cJSON_IsBool(param)) {
                if (cJSON_IsTrue(param)) {
                    restartWifiClient(NULL);
                }
            }
            break;
        case UPDATE:
            param = cJSON_GetObjectItem(command->data, "version");
            if (param && cJSON_IsString(param)){
                if (strcmp(getCurrentVersion(), cJSON_GetStringValue(param)) != 0){
                    launchUpdate();
                }
            }
            break;
        }
        cJSON_Delete(command->data);
        free(command);
    }
    vTaskDelete(NULL);
}

esp_err_t	launchCommand(MqttClientCommand_t *command)
{
    MqttClientCommand_t	*tmp = NULL;

    ESP_LOGI(TAG, "Launching command...");
    if (!command)
        return ESP_FAIL;
    tmp = (MqttClientCommand_t *)malloc(sizeof(MqttClientCommand_t));
    if (!tmp)
        return ESP_FAIL;
    memcpy(tmp, command, sizeof(MqttClientCommand_t));
    return xTaskCreate(&taskCommandHandler, "commandHandler", 4098, tmp, tskIDLE_PRIORITY, NULL);
}