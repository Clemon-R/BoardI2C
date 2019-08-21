#include "SensorClient.h"
#include "../mqtt/MqttClient.h"
#include "cJSON.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "../Main.h"
#include "driver/gpio.h"

static const char	*TAG = "\033[1;94mSensorClient\033[0m";
static TaskHandle_t	sensorTask = NULL;

static char	_running = false;
static char _working = false;
static QueueHandle_t	_datas;
static SensorData_t		*_config = NULL;

static TickType_t	_refreshDelai = pdMS_TO_TICKS(5000);
static SensorValues_t   _values = {0,0,0,100,(color_rgb_t){0,0,0,0},0};
static esp_adc_cal_characteristics_t    *_adc_chars = NULL;

struct Tension_s
{
    uint32_t   value;
    uint8_t percentage;
};
DRAM_ATTR static struct Tension_s    _tensionSamples[] = {
    {0, 0}, 
    {3000, 0}, 
    {3300, 5}, 
    {3600, 10}, 
    {3700, 20}, 
    {3750, 30},
    {3790, 40},
    {3830, 50},
    {3870, 60},
    {3920, 70},
    {3970, 80},
    {4100, 90},
    {4200, 100}
    };

//With the table and calcul getting the current percentage of battery
static int8_t    getBatteryPercentage()
{
    uint32_t    adc_reading = 0;

    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        adc_reading += adc1_get_raw((adc1_channel_t)ADC_CHANNEL);
    }
    adc_reading /= NO_OF_SAMPLES;

    uint32_t    voltage = esp_adc_cal_raw_to_voltage(adc_reading, _adc_chars);
    uint32_t    realVoltage = REAL_VOLTAGE(voltage);
    //printf("Voltage: %dmV, RealVolateg: %dmV\n", voltage, realVoltage);
    for (int i = 1;i < NBR_TENSION_SAMPLE;i++) //index from 1 because we need min and max, if index 0 we don't forcefully
    {
        uint32_t min = _tensionSamples[i - 1].value;
        uint32_t max = _tensionSamples[i].value;
        if (realVoltage >= min && realVoltage <= max){
            return _tensionSamples[i].percentage;
        }
    }
    return -1; //Not in the scope
}

//Init the BUS I2C for the sensors
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

//Removing the BUS I2C for sensors
static esp_err_t	nordicI2CDeinit()
{
    i2c_port_t i2c_master_port = I2C_MASTER_NUM;


    ESP_LOGI(TAG, "Deinitiating I2C of master...");
    return i2c_driver_delete(i2c_master_port);
}

//Calibrating all the sensors and checking WHOIAM
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

//Setting up the GPIO ADC to handle battery
static esp_err_t    setupAdc()
{
    esp_err_t   ret;

    ret = adc1_config_width(ADC_WIDTH_BIT_12);
    if (ret != ESP_OK)
        return ret;
    ret = adc1_config_channel_atten((adc1_channel_t)ADC_CHANNEL, ADC_ATTEN);  
    _adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, DEFAULT_VREF, _adc_chars);
    return ret;
}

//Process
static void	taskSensor(void *args)
{
    SensorData_t	data;
    char	buffer[BUFF_SIZE];
    char    batteryLow = false, alertSensors = false;

    ESP_LOGI(TAG, "Initiating the task...");
    gpio_set_level(RGB_3_RED, RGB_ON);
    ESP_ERROR_CHECK(nordicI2CInit());
    ESP_ERROR_CHECK(setupAdc());
    while (_running) {
        vTaskDelay(_refreshDelai);
        _values.battery = getBatteryPercentage();
        if (_values.battery <= 20 && !batteryLow){
            createAlert("battery_low", "The battery is under 20%", MAJOR, false);
            batteryLow = true;
        } else if (_values.battery > 20 && batteryLow){
            createAlert("battery_low", "The battery is under 20%", MAJOR, true);
            batteryLow = false;
        }
        if (!_values.initiated) {
            if (setupAllSensors(&data) != ESP_OK) {
                _config = NULL;
                ESP_LOGE(TAG, "Impossible to notify and callibrate sensors");
                gpio_set_level(RGB_3_RED, RGB_OFF);
                gpio_set_level(RGB_3_GREEN, !_values.initiated);
                gpio_set_level(RGB_3_BLUE, _values.initiated);
                createAlert("sensors_error", "Impossible to callibrate or not valid sensors", MAJOR, false);
                alertSensors = true;
                continue;
            } else {
                _config = &data;
                if (alertSensors){
                    createAlert("sensors_error", "Impossible to callibrate or not valid sensors", MAJOR, true);
                    alertSensors = false;
                }
            }
        }
        _values.color = getColorRGB(I2C_MASTER_NUM);
        _values.temperature = getTemperature(I2C_MASTER_NUM, &data.humidityData);
        _values.humidity = getHumidity(I2C_MASTER_NUM, &data.humidityData);
        _values.pressure = getPressure(I2C_MASTER_NUM);
        if (_values.temperature == (float)-1 || _values.humidity == (float)-1 || _values.pressure == (float)-1 || _values.color.available == 0)
            _values.initiated = false;
        else
            _values.initiated = true;
        _working = _values.initiated;
        gpio_set_level(RGB_3_RED, RGB_OFF);
        gpio_set_level(RGB_3_GREEN, !_values.initiated);
        gpio_set_level(RGB_3_BLUE, _values.initiated);
        if (xQueueIsQueueFullFromISR(_datas) == pdTRUE)
            continue;
        cJSON	*monitor = cJSON_CreateObject();
        if (!monitor)
            continue;

        cJSON	*sensors = cJSON_CreateObject();

        //Température
        cJSON	*raw = cJSON_CreateNumber(_values.temperature);
        cJSON_AddItemToObject(sensors, "temperature", raw);

        //Humidité
        raw = cJSON_CreateNumber(_values.humidity);
        cJSON_AddItemToObject(sensors, "humidity", raw);

        //Pression
        raw = cJSON_CreateNumber(_values.pressure);
        cJSON_AddItemToObject(sensors, "pressure", raw);

        //Color
        sprintf(buffer, "0X%02X,0X%02X,0X%02X", _values.color.r & 0XFF, _values.color.g & 0XFF, _values.color.b & 0XFF);
        raw = cJSON_CreateString(buffer);
        cJSON_AddItemToObject(sensors, "color", raw);

        cJSON_AddStringToObject(monitor, "type", "sensor_data");
        cJSON_AddItemToObject(monitor, "value", sensors);

        xQueueSend(_datas, &monitor, 10);
    }
    nordicI2CDeinit();
    _config = NULL;
    _running = false;
    gpio_set_level(RGB_3_RED, RGB_ON);
    gpio_set_level(RGB_3_GREEN, RGB_OFF);
    vTaskDelete(sensorTask);
}

//Launching the task with required data
esp_err_t	startSensorClient()
{
    if (_running)
        return ESP_FAIL;
    _datas = getQueueDatas();
    if (!_datas)
        return ESP_FAIL;
    _running = true;
    _working = false;
    return xTaskCreate(taskSensor, "sensorTask", 3072, NULL, tskIDLE_PRIORITY, &sensorTask);;
}

//Changing state of the process to go to the end of the process
esp_err_t	stopSensorClient()
{
    if (!_running)
        return ESP_FAIL;
    _running = false;
    return ESP_OK;
}

//Changing tick of refresh data sensors
void	setRefreshDelai(TickType_t value)
{
    _refreshDelai = value;
}

//Getting the current tick of refresh data sensors
TickType_t  getRefreshDelai()
{
    return _refreshDelai;
}

//Checking the state of the process
char	isSensorRunning()
{
    return _running;
}

//Checking the state of the sensors (false also when not init)
char	isSensorWorking()
{
    return _working;
}

//Getting all the sensors config
SensorData_t	*getSensorConfig()
{
    return _config;
}

//Getting the current values of all sensors
SensorValues_t  getSensorValues()
{
    return _values;
}
