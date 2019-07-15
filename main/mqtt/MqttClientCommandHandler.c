#include "MqttClientCommandHandler.h"

#include "driver/gpio.h"
#include "cJSON.h"

#include "../Main.h"

#include "../wifi/WifiClient.h"
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
        case LED:
            param = cJSON_GetObjectItem(command->data, "state");
            if (!param || !cJSON_IsBool(param))
                break;
            gpio_set_level(RGB_1, cJSON_IsTrue(param) ? 1 : 0);
            gpio_set_level(RGB_2, cJSON_IsTrue(param) ? 1 : 0);
            gpio_set_level(RGB_3, cJSON_IsTrue(param) ? 1 : 0);
            break;

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