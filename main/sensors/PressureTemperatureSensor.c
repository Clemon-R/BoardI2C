#include "PressureTemperatureSensor.h"
#include "../I2CByteManager.h"

static const char TAG[] = "\033[1;94mPressureTemperatureSensor\033[0m";

esp_err_t	initPressureTempSensor(i2c_port_t port)
{
    uint8_t data;
    int ret;

    ESP_LOGI(TAG, "Asking WHOIAM for sensor Pressure/Temperature...");
    ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEMP_WHOIAM, NULL);
    if (ret != ESP_OK)
        return ESP_FAIL;
    ret = readByte(port, I2C_SLAVE_PRESSURETEMP, &data);
    if (ret != ESP_OK || data != PRESSURETEMP_WHOIAM_ANS)
        return ESP_FAIL;
    return ret;
}


esp_err_t	setupPressureTempSensor(i2c_port_t port, pressure_temp_sensor_t *result)
{
    uint8_t	data;
    esp_err_t	ret;

    if (!result)
        return ESP_FAIL;
    data = PRESSURETEMP_ODR;
    ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEMP_CTRL_REG1, &data);
    if (ret != ESP_OK)
        return ret;

    ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEP_RPDSL, (uint8_t *)&(result->offset));
    if (ret != ESP_OK)
        return ret;

    return ESP_OK;
}


float	getPressure(i2c_port_t port)
{
    int ret;
    int32_t	result = 0;
    int16_t	offset = 0;

    ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, PRESSURETEP_PRESS_OUTXL, NULL);
    if (ret != ESP_OK)
        return -1;
    ret = readBytes(port, I2C_SLAVE_PRESSURETEMP, (uint8_t *)&result, 3);
    if (ret != ESP_OK)
        return -1;

    ret = writeByte(port, I2C_SLAVE_PRESSURETEMP, 0x18, NULL);
    if (ret != ESP_OK)
        return -1;
    ret = readBytes(port, I2C_SLAVE_PRESSURETEMP, (uint8_t *)&offset, 2);
    if (ret != ESP_OK)
        return -1;
    return result / 4096.0f;
}