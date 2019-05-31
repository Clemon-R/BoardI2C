
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/i2c.h"

#include "I2CByteManager.h"

//Sensor
#define	I2C_SLAVE_ADD1	0x68
#define	I2C_SLAVE_ADD2	0x69
#define	I2C_MASTER_NUM	(i2c_port_t)0
#define I2C_MASTER_SDA_IO	(gpio_num_t)4
#define I2C_MASTER_SCL_IO	(gpio_num_t)0
#define I2C_MASTER_FREQ_HZ	100000

static char	TAG[] = "Main";

esp_err_t nordicI2CInit()
{
    i2c_port_t i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;

	ESP_LOGI(TAG, "Init I2C for master...");
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(i2c_master_port, &conf);
    return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
}

void	app_main(){
	ESP_ERROR_CHECK(nordicI2CInit());
}