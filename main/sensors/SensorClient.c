#include "SensorClient.h"

static const char	*TAG = "SensorClient";
static TaskHandle_t	sensorTask = NULL;

static char	_running = false;

static esp_err_t	nordicI2CInit()
{
	i2c_port_t i2c_master_port = I2C_MASTER_NUM;
	i2c_config_t conf;

	ESP_LOGI(TAG, "Initiating I2C of master...");
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = I2C_MASTER_SDA_IO;
	conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
	conf.scl_io_num = I2C_MASTER_SCL_IO;
	conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
	conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

	i2c_param_config(i2c_master_port, &conf);
	if (i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0) != ESP_OK){
		ESP_LOGE(TAG, "Failed to init I2C for master");
		return ESP_FAIL;
	}
	ESP_LOGI(TAG, "Successfull initiated I2C for master");
	return ESP_OK;
}

static esp_err_t	nordicI2CDeinit()
{
	i2c_port_t i2c_master_port = I2C_MASTER_NUM;


	ESP_LOGI(TAG, "Deinitiating I2C of master...");
	return i2c_driver_delete(i2c_master_port);
}

static esp_err_t	setupAllSensors(SensorData_t *data)
{
	esp_err_t	ret;

	ESP_LOGI(TAG, "Setting all sensors...");
	if (!data)
		return ESP_FAIL;
	ret = initHumidityTempSensor(I2C_MASTER_NUM);
	if (ret != ESP_OK)
		return ret;
	ret = initPressureTempSensor(I2C_MASTER_NUM);
	if (ret != ESP_OK)
		return ret;
	ret = initColorSensor(I2C_MASTER_NUM);
	if (ret != ESP_OK)
		return ret;
	ret = setupPressureTempSensor(I2C_MASTER_NUM, &(data->pressureData));
	if (ret != ESP_OK)
		return ret;
	ret = setupHumidityTempSensor(I2C_MASTER_NUM, &(data->humidityData));
	if (ret != ESP_OK)
		return ret;
	ret = setupColorSensor(I2C_MASTER_NUM);
	return ret;
}

static void	taskSensor(void *args)
{
	SensorData_t	data;

    ESP_LOGI(TAG, "Initiating the task...");
	ESP_ERROR_CHECK(nordicI2CInit());
	ESP_ERROR_CHECK(setupAllSensors(&data));
	while (_running){
		ESP_LOGI(TAG, "Result Temperature: %d°C", getTemperature(I2C_MASTER_NUM, &data.humidityData));
		ESP_LOGI(TAG, "Result Humidity: %d%c", getHumidity(I2C_MASTER_NUM, &data.humidityData), '%');
		ESP_LOGI(TAG, "Result Pressure: %dhPa", getPressure(I2C_MASTER_NUM));

		color_rgb_t tmp = getColorRGB(I2C_MASTER_NUM);
		ESP_LOGI(TAG, "Result RGB R: 0x%2x, G: 0x%2x, B: 0x%2x", tmp.r, tmp.g, tmp.r);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	ESP_ERROR_CHECK(nordicI2CDeinit());
	vTaskDelete(NULL);
}

esp_err_t	startSensorClient()
{
	if (_running)
		return ESP_FAIL;
	_running = true;
	return xTaskCreate(taskSensor, "sensorTask", 4098, NULL, tskIDLE_PRIORITY, &sensorTask);;
}

esp_err_t	stopSensorClient()
{
	if (!_running)
		return ESP_FAIL;
	_running = false;
	return ESP_OK;
}