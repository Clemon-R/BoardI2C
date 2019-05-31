#ifndef	I2CBYTEMANAGER_H_
#define	I2CBYTEMANAGER_H_

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "esp_err.h"
#include "driver/i2c.h"

#define ACK_VAL	(i2c_ack_type_t)0x0                             /*!< I2C ack value */
#define NACK_VAL	(i2c_ack_type_t)0x1                            /*!< I2C nack value */
#define ACK_CHECK_EN	0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS	0x0                       /*!< I2C master will not check ack from slave */

#define WRITE_BIT 0              /*!< I2C master write */
#define READ_BIT 1                /*!< I2C master read */

//Display buffer
void	displayerBuffer(uint8_t *buf, int len);

//ReadBytes on I2C port
esp_err_t	readByte(i2c_port_t i2c_num, uint8_t address, uint8_t *data);
esp_err_t	readBytes(i2c_port_t i2c_num, uint8_t address, uint8_t *data, size_t size);

//WriterBytes on I2C port
esp_err_t	writeByte(i2c_port_t i2c_num, uint8_t address, uint8_t registerId, uint8_t *data);
esp_err_t	writeBytes(i2c_port_t i2c_num, uint8_t address, uint8_t registerId, uint8_t *data, size_t size);

#endif