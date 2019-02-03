/*
 * I2C.hpp
 *
 *  Created on: Nov 25, 2018
 *      Author: Jasper
 */

#ifndef I2C_H_
#define I2C_H_

#include <stdio.h>
#include <string.h>
#include "sci.h"

#include "i2c.h"
#include "FreeRTOS.h"
#include "os_semphr.h"
#include "os_task.h"

#define I2C_DEFAULT         i2cREG1
#define I2C_TIMEOUT_MAX     200000
#define I2C_MAX_RESETS      5
#define I2C_MUTEX_ATTEMPTS  20
#define I2C_MUTEX_WAIT      100
#define I2C_TIMEOUT_FAIL    -30077
#define I2C_ERR_NACK        -30066
#define I2C_BUSBUSY_FAIL    -30055
#define I2C_TXREADY_FAIL    -30044
#define I2C_MUTEX_FAIL      -30033
#define I2C_OK              1

void I2C_init(i2cBASE_t *i2c);

int16_t I2C_send(uint32_t length, uint8_t *data, uint32_t addr);
static int16_t _I2C_send(uint32_t length, uint8_t *data);
int16_t I2C_receive(uint32_t clength, uint8_t* cmd, uint32_t dlength, uint8_t *data, uint32_t addr);
static int16_t _I2C_receive(uint32_t length, uint8_t *data);
int16_t I2C_is_bus_busy();
int16_t I2C_ok_transmit();
int16_t I2C_reset();

static int16_t I2C_set_slave_addr(uint32_t addr);
static bool I2C_get_mutex();

static i2cBASE_t *I2C_i2c;

static uint8_t I2C_num_resets;
static xSemaphoreHandle I2CMutex;
static bool I2C_initialized = false;

#endif /* I2C_HPP_ */
