#include "SensorClient.h"
#include "../mqtt/MqttClient.h"
#include "cJSON.h"

#include "../Main.h"
#include "driver/gpio.h"

static const char	*TAG = "\033[1;94mSensorClient\033[0m";
static TaskHandle_t	sensorTask = NULL;

static char	_running = false;
static char _working = false;
static QueueHandle_t	_datas;
static SensorData_t		*_config = NULL;

static TickType_t	refreshDelai = pdMS_TO_TICKS(1000);

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
    if (i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0) != ESP_OK) {
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
    char	buffer[BUFF_SIZE];
    TickType_t	waitingTicks = pdMS_TO_TICKS(3000);
    char	state = 0;

    ESP_LOGI(TAG, "Initiating the task...");
    gpio_set_level(RGB_3, 0);
    ESP_ERROR_CHECK(nordicI2CInit());
    while (_running) {
        vTaskDelay(refreshDelai);
        if (!state) {
            if (setupAllSensors(&data) != ESP_OK)
            {
                _config = NULL;
                ESP_LOGE(TAG, "Impossible to notify and callibrate sensors");
            }
            else
            {
                state = 1;
                _config = &data;
            }
        }
        float	temp, humidity, pressure;
        color_rgb_t color = getColorRGB(I2C_MASTER_NUM);
        temp = getTemperature(I2C_MASTER_NUM, &data.humidityData);
        humidity = getHumidity(I2C_MASTER_NUM, &data.humidityData);
        pressure = getPressure(I2C_MASTER_NUM);
        if (temp == (float)-1 || humidity == (float)-1 || pressure == (float)-1 || color.available == 0)
            state = 0;
        ESP_LOGI(TAG, "temperature: %f, humidity: %f, pressure: %f, color: %d, state: %d", temp, humidity, pressure, color.available, state);
        _working = state;
        gpio_set_level(RGB_3, state);
        cJSON	*monitor = cJSON_CreateObject();
        if (!monitor)
            continue;

        cJSON	*sensors = cJSON_CreateObject();

        //Température
        cJSON	*raw = cJSON_CreateNumber(temp);
        cJSON_AddItemReferenceToObject(sensors, "temperature", raw);

        //Humidité
        raw = cJSON_CreateNumber(humidity);
        cJSON_AddItemReferenceToObject(sensors, "humidity", raw);

        //Pression
        raw = cJSON_CreateNumber(pressure);
        cJSON_AddItemReferenceToObject(sensors, "pressure", raw);

        //Color
        sprintf(buffer, "0X%02X,0X%02X,0X%02X", color.r & 0XFF, color.g & 0XFF, color.b & 0XFF);
        raw = cJSON_CreateString(buffer);
        cJSON_AddItemReferenceToObject(sensors, "color", raw);

        cJSON_AddItemReferenceToObject(monitor, "sensors", sensors);

        while (xQueueIsQueueFullFromISR(_datas) == pdTRUE) {
            vTaskDelay(waitingTicks);
        }
        xQueueSend(_datas, &monitor, waitingTicks);
    }
    nordicI2CDeinit();
    _config = NULL;
    _running = false;
    gpio_set_level(RGB_3, 0);
    sensorTask = NULL;
    vTaskDelete(NULL);
}

esp_err_t	startSensorClient()
{
    if (_running)
        return ESP_FAIL;
    _datas = getQueueDatas();
    _running = true;
    _working = false;
    return xTaskCreate(taskSensor, "sensorTask", 10240, NULL, tskIDLE_PRIORITY, &sensorTask);;
}

esp_err_t	stopSensorClient()
{
    if (!_running)
        return ESP_FAIL;
    _running = false;
    return ESP_OK;
}

void	setRefreshDelai(TickType_t value)
{
    refreshDelai = value;
}

char	isSensorRunning()
{
    return _running;
}

char	isSensorWorking()
{
    return _working;
}

SensorData_t	*getSensorConfig()
{
    return _config;
}