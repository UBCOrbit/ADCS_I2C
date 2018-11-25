/*
 * I2C.hpp
 *
 *  Created on: Nov 25, 2018
 *      Author: Jasper
 */

#ifndef I2C_HPP_
#define I2C_HPP_

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

class I2C
{
public:
    I2C();
    I2C(i2cBASE_t *i2c);
    int16_t send(uint32_t length, uint8_t *data, uint32_t addr);
    int16_t send(uint32_t length, uint8_t *data);
    int16_t _send(uint32_t length, uint8_t *data);
    int16_t receive(uint32_t clength, uint8_t* cmd, uint32_t dlength, uint8_t *data, uint32_t addr);
    int16_t receive(uint32_t clength, uint8_t* cmd, uint32_t dlength, uint8_t *data);
    int16_t _receive(uint32_t length, uint8_t *data);
    int16_t is_bus_busy();
    int16_t ok_transmit();
    int16_t reset();
    virtual ~I2C();
private:
    void I2CInit(i2cBASE_t *i2c);
    int16_t set_slave_addr(uint32_t addr);
    bool get_mutex();

    i2cBASE_t *i2c;

    static uint8_t num_resets;
    static xSemaphoreHandle I2CMutex;
};

#endif /* I2C_HPP_ */
