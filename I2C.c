/*
 * I2C.cpp
 *
 *  Created on: Nov 25, 2018
 *      Author: Jasper
 */

#include "I2C.h"
void I2C_init(i2cBASE_t *i) 
{
    if(I2C_initialized)
        return;

    // TODO: Figure out if mutex can be instantiated with a 
    // static member function instead
    if(I2CMutex == NULL) {
        I2CMutex = xSemaphoreCreateMutex();
    }

    I2C_i2c = i;
    i2cInit();

    I2C_num_resets = 0;
    I2C_initialized = true;
}

int16_t I2C_send(uint32_t length, uint8_t *data, uint32_t addr)
{
    if(I2C_set_slave_addr(addr) !=I2C_OK)
        return I2C_MUTEX_FAIL;

    int16_t err;
    if(!I2C_get_mutex())
        return I2C_MUTEX_FAIL;
    i2cSetDirection(I2C_i2c, I2C_TRANSMITTER);
    i2cSetCount(I2C_i2c, length);
    i2cSetMode(I2C_i2c, I2C_MASTER);
    i2cSetStop(I2C_i2c);
    i2cSetStart(I2C_i2c);

    err = _I2C_send(length, data);
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }

    i2cSetStop(I2C_i2c);
    i2cClearSCD(I2C_i2c);

    err = I2C_is_bus_busy();
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }

    err = I2C_ok_transmit();
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }

    xSemaphoreGive(I2CMutex);
    return I2C_OK;
}

int16_t _I2C_send(uint32_t length, uint8_t *data)
{
    uint32_t timeout = 0;

    while(length > 0U){
        // Wait while transmit (I2C_TX_INT) and access 
        // (I2C_ARDY_INT) are not ready
        while(((I2C_i2c->STR & (uint32_t) I2C_TX_INT 
                        | (uint32_t) I2C_ARDY_INT)
                    == 0U)
                && timeout < I2C_TIMEOUT_MAX)
        {
            timeout++;
        }

        if(timeout >= I2C_TIMEOUT_MAX)
            return I2C_TIMEOUT_FAIL;

        if(I2C_i2c->STR & (uint32_t) I2C_NACK_INT) {
            i2cSetStop(I2C_i2c);
            I2C_i2c->STR = I2C_NACK_INT;
            return I2C_ERR_NACK;
        }

        I2C_i2c->DXR = (uint32_t) (*data);
        data++;
        length--;
    }
    return I2C_OK;
}

int16_t I2C_receive(uint32_t clength, 
                uint8_t* cmd, 
                uint32_t dlength, 
                uint8_t *data, 
                uint32_t addr)
{
    if(I2C_set_slave_addr(addr) != I2C_OK)
        return I2C_MUTEX_FAIL;

    char hex_buffer[3];
    char hex_string_buffer[64];
    char debug_buffer[64];

    uint32_t i,j;

    for(i=0; i<clength; i++){
        sprintf(hex_buffer, "%02x", cmd[i]);
        for(j=0; j<2; j++){
            sprintf(hex_string_buffer+i*2+j, "%c", hex_buffer[j]);
        }
    }

    sprintf(debug_buffer, "[I2C] Sending: %s to %d\r\n", hex_string_buffer, addr);

    sciSafeSend(scilinREG, strlen(debug_buffer), debug_buffer);


    int16_t err;
    if(!I2C_get_mutex())
        return I2C_MUTEX_FAIL;
    i2cSetDirection(I2C_i2c, I2C_TRANSMITTER);
    i2cSetCount(I2C_i2c, clength);
    i2cSetMode(I2C_i2c, I2C_MASTER);
    i2cSetStop(I2C_i2c);
    i2cSetStart(I2C_i2c);

    err = _I2C_send(clength, cmd);
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }

    i2cSetDirection(I2C_i2c, I2C_RECEIVER);
    i2cSetCount(I2C_i2c, dlength);
    i2cSetMode(I2C_i2c, I2C_MASTER);
    err = _I2C_receive(dlength, data);
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }
    i2cSetStop(I2C_i2c);
    i2cClearSCD(I2C_i2c);

    err = I2C_is_bus_busy();
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }

    err = I2C_ok_transmit();
    if(err != I2C_OK) {
        xSemaphoreGive(I2CMutex);
        return err;
    }

    xSemaphoreGive(I2CMutex);
    return I2C_OK;
}

int16_t _I2C_receive(uint32_t length, uint8_t *data)
{
    uint32_t timeout = 0;

    while(length > 0U){
        // Wait while receive (I2C_RX_INT) is not ready
        while(((I2C_i2c->STR & (uint32_t) I2C_RX_INT)
                    == 0U)
                && timeout < I2C_TIMEOUT_MAX)
        {
            timeout++;
        }

        if(timeout >= I2C_TIMEOUT_MAX)
            return I2C_TIMEOUT_FAIL;

        if(I2C_i2c->STR & (uint32_t) I2C_NACK_INT) {
            i2cSetStop(I2C_i2c);
            I2C_i2c->STR = I2C_NACK_INT;
            return I2C_ERR_NACK;
        }

        *data = ((uint8_t) I2C_i2c->DRR);
        data++;
        length--;
    }
    return I2C_OK;
}

int16_t I2C_is_bus_busy()
{
    uint32_t timeout = 0;
    while((I2C_i2c->STR & I2C_BUSBUSY) 
            && timeout < I2C_TIMEOUT_MAX)
    {
        timeout++;
    }
    if(timeout >= I2C_TIMEOUT_MAX)
        return I2C_BUSBUSY_FAIL;
    return I2C_OK;
}

int16_t I2C_ok_transmit()
{
    uint32_t timeout = 0;
    while((I2C_i2c->MDR & I2C_MASTER)
            && timeout < I2C_TIMEOUT_MAX)
    {
        timeout++;
    }
    if(timeout >= I2C_TIMEOUT_MAX)
        return I2C_TIMEOUT_FAIL;
    return I2C_OK;
}

int16_t I2C_reset()
{
    if(!I2C_get_mutex())
        return I2C_MUTEX_FAIL;

    uint8_t i,j;
    vTaskSuspendAll();
    do {
        I2C_i2c->MDR = (uint32_t)((uint32_t)0U << 5U);
        I2C_i2c->PFNC = (0x01);
        I2C_i2c->DIR = (0x01);

        for(i = 0; i < 10; i++) {
            I2C_i2c->DOUT = I2C_i2c->DOUT | 0x01;
            for(j=0;j<300;j--);
            I2C_i2c->DOUT ^= I2C_i2c->DOUT;
            for(j=0;j<300;j--);
        }
        I2C_num_resets++;
    } while(I2C_i2c->DIN & 0x02 != 0x02 && I2C_num_resets < I2C_MAX_RESETS);

    if(I2C_num_resets >= 5) {
        xSemaphoreGive(I2CMutex);
        while(1);
    }

    I2C_init(I2C_i2c);
    xTaskResumeAll();
    return I2C_OK;
}


bool I2C_get_mutex()
{
    bool got_mutex = false;
    uint8_t mutex_tries = 0;
    while(!got_mutex && mutex_tries < I2C_MUTEX_ATTEMPTS) {
        got_mutex = xSemaphoreTake(I2CMutex, I2C_MUTEX_WAIT);
        mutex_tries++;
        if(!got_mutex)
            vTaskDelay(10);
    }
    return got_mutex;
}

int16_t I2C_set_slave_addr(uint32_t addr) {
    if(!I2C_get_mutex())
        return I2C_MUTEX_FAIL;
    i2cSetSlaveAdd(I2C_i2c, addr);
    xSemaphoreGive(I2CMutex);
    return I2C_OK;
}
