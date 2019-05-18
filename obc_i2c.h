#ifndef OBC_I2C_H_
#define OBC_I2C_H_

#include "i2c.h"
#include "FreeRTOS.h"
#include "rtos_semphr.h"

extern SemaphoreHandle_t xI2CMutex;

#define I2C i2cREG1

// I2C COMMANDS
#define RD_CMD 0x1
#define WR_CMD 0x0

// I2C error handling
#define I2C_TIMEOUT_MAX 	200000			// number of attempts before timing out (units are attempts at register check), typically takes ~3500 for a correct wait
#define I2C_TIMEOUT_FAIL 	-30077			// if we timeout while waiting for something
#define I2C_ERR_NACK 		-30066			// if we receive an unexpected NACK
#define I2C_BUSBUSY_FAIL	-30055			// bus is busy (both lines aren't high)
#define I2C_TXREADY_FAIL	-30044			// bit indicating we're ok to transmit again did not go low
#define I2C_OK 				1				// I2C functions return this when everything is good

void i2c_init();
void i2c_setup_transaction(i2cBASE_t *i2c, uint8_t addr, uint32 dir, uint32_t num_bytes);
int16_t i2c_reset(i2cBASE_t *i2c);
int16_t i2c_get_status();

int16_t i2c_ping_device(uint8_t addr);
bool i2c_ping_device_zero_transfer(uint8_t addr);
int16_t i2c_send(i2cBASE_t *i2c, uint32 length, uint8 * data);
int16_t i2c_receive(i2cBASE_t *i2c, uint32 length, uint8 * data);
int16_t i2c_is_bus_busy();
int16_t i2c_is_ok_transmit();
int16_t i2c_check_nack(i2cBASE_t *i2c, bool clear);

#endif /* OBC_I2C_H_ */
