#include "I2CByteManager.h"
#include "esp_log.h"

//Display
void	displayerBuffer(uint8_t *buf, int len)
{
    int	i;

    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

//Readers
esp_err_t	readBytes(i2c_port_t i2c_num, uint8_t address, uint8_t *data, size_t size)
{
    if (size == 0 || data == NULL) {
        return ESP_FAIL;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | READ_BIT, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t	readByte(i2c_port_t i2c_num, uint8_t address, uint8_t *data)
{
    if (data == NULL) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, data, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

//Writers
esp_err_t	writeByte(i2c_port_t i2c_num, uint8_t address, uint8_t registerId, uint8_t *data)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, registerId, ACK_CHECK_EN);
    if (data != NULL) {
        i2c_master_write_byte(cmd, *data, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t	writeBytes(i2c_port_t i2c_num, uint8_t address, uint8_t registerId, uint8_t *data, size_t size)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, registerId, ACK_CHECK_EN);
    if (size != 0 && data != NULL) {
        i2c_master_write(cmd, data, size, ACK_CHECK_EN);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}