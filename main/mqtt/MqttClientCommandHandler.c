#include "MqttClientCommandHandler.h"

static const char	*TAG = "\033[1;36mMqttClientCommandHandler\033[0m";

static void	taskCommandHandler(void *args)
{
	MqttClientCommand_t	*command = (MqttClientCommand_t *)args;

	ESP_LOGI(TAG, "Starting the task for the command...");
	if (command){
		switch (command->type){
			case CHANGEWIFI:
			ESP_LOGI(TAG, "Changing the current wifi...");
			break;
		}
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