#include "obc_i2c.h"
#include "obc_uart.h"
#include "obc_utils.h"
#include "i2c.h"
#include "FreeRTOS.h"
#include "rtos_semphr.h"
#include "rtos_task.h"

SemaphoreHandle_t xI2CMutex;
static uint8_t num_resets;

void i2c_init() {
	i2cInit();
	num_resets = 0;
	xI2CMutex = xSemaphoreCreateMutex();
	if (xI2CMutex != NULL) {
		/* The semaphore was created successfully and
		 can be used. */
	} else {
		i2c_reset(i2cREG1);
	}
}

int16_t i2c_get_status() {
	int16_t errcode = 0;
	errcode = i2c_is_bus_busy();
	if (errcode != I2C_OK)
		return errcode;

	errcode = i2c_is_ok_transmit();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int16_t i2c_is_bus_busy() {
	uint32_t timeout = 0;
	while ((I2C->STR & I2C_BUSBUSY) && timeout < I2C_TIMEOUT_MAX) {
		timeout++;
	}
	if (timeout >= I2C_TIMEOUT_MAX)
		return I2C_BUSBUSY_FAIL;
	return I2C_OK;
}

int16_t i2c_is_ok_transmit() {
	uint32_t timeout = 0;
	while ((I2C->MDR & I2C_MASTER) && timeout < I2C_TIMEOUT_MAX) {// goes low when we're good to transmit again 	[TRM pg. 1498]
		timeout++;
	}
	if (timeout >= I2C_TIMEOUT_MAX)
		return I2C_TXREADY_FAIL;
	return I2C_OK;
}

bool i2c_is_nack(i2cBASE_t *i2c, bool clear) {
	/* If a NACK occurred, SCL is held low and STP bit cleared: http://processors.wiki.ti.com/index.php/I2C_Tips */
	if (i2c->STR & (uint32_t) I2C_NACK_INT) {
		i2cSetStop(i2cREG1);
		if (clear) {
			i2c->STR = I2C_NACKSNT; // write 1 to clear
		}
		return true;
	} else {
		return false;
	}
}

void i2c_setup_transaction(i2cBASE_t *i2c, uint8_t addr, uint32 dir, uint32_t num_bytes) {
	i2cSetSlaveAdd(i2c, addr);
	i2cSetDirection(i2c, dir);
	i2cSetCount(i2c, num_bytes);
	i2cSetMode(i2c, I2C_MASTER);
}

int16_t i2c_ping_device(uint8_t addr) {
	int bytes_send = 1;
	uint8_t cmd = (addr << 1 | RD_CMD);
	i2c_setup_transaction(i2cREG1, addr, I2C_TRANSMITTER, bytes_send);

	i2cSetStart(i2cREG1);
	int16_t errcode = i2c_send(i2cREG1, bytes_send, &cmd);

	// Wait for "TX" ready flag to transmit data or "ARDY" if we get NACKed
	while ( !(i2cREG1->STR & ((uint32) I2C_TX | (uint32_t) I2C_ARDY)) );

	if (i2c_is_nack(i2cREG1, true)) {
		serial_send_ln("I2C NACK received\n");
		errcode = I2C_ERR_NACK;
	} else if (errcode == I2C_OK ){
		serial_send_ln("I2C received ACK\n");
	}

	return errcode;
}

bool i2c_ping_device_zero_transfer(uint8_t addr) {
	uint32_t i2cMdrBu = i2cREG1->MDR;
	i2cREG1->MDR |= (I2C_REPEATMODE);
	i2cSetSlaveAdd(i2cREG1, addr);
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);
	i2cSetCount(i2cREG1, 0);
	i2cSetMode(i2cREG1, I2C_MASTER);
	i2cSetStart(i2cREG1);
	// SCD flag is set after an ACK from device, ARDY flag is set after NACK is detected
	while ( !(i2cREG1->STR & ((uint32_t) I2C_ARDY | (uint32_t) I2C_SCD)) );
	//while ( !(i2cREG1->STR & ((uint32_t) I2C_ARDY )) );
	if (i2c_is_nack(i2cREG1, true)) {
		i2cREG1->MDR = i2cMdrBu;
		return false;
	} else {
		i2cREG1->MDR = i2cMdrBu;
		return true;
	}
}

int16_t i2c_send(i2cBASE_t *i2c, uint32 length, uint8 * data) {
	uint32_t timeout_count = 0;
	while (length > 0U) {
		while (((i2c->STR & (uint32) I2C_TX_INT | (uint32) I2C_ARDY_INT) == 0U) && (timeout_count < I2C_TIMEOUT_MAX)) {
			timeout_count++;
		}

		if (timeout_count >= I2C_TIMEOUT_MAX) {
			return I2C_TIMEOUT_FAIL;
		}

		/* If a NACK occurred, SCL is held low and STP bit cleared: http://processors.wiki.ti.com/index.php/I2C_Tips */
		if (i2c->STR & (uint32_t) I2C_NACK_INT) {
			i2cSetStop(i2cREG1);
			i2c->STR = I2C_NACKSNT; // write 1 to clear
			return I2C_ERR_NACK;
		}

		i2c->DXR = (uint32) (*data);
		data++;
		length--;
	}
// these don't work here
//	while (I2C->STR & I2C_BUSBUSY);	// goes low when stop has gone through			[TRM pg. 1492]
//	while (I2C->MDR & I2C_MASTER);	// goes low when we're good to transmit again 	[TRM pg. 1498]
	return I2C_OK;
}

int16_t i2c_receive(i2cBASE_t *i2c, uint32 length, uint8 * data) {
	uint32_t timeout_count;
	timeout_count = 0;

	// maybe do ARDY up here

	while (length > 0U) {
		// It doesn't seem to like a check for ARDY
		while (((i2c->STR & (uint32) I2C_RX_INT) == 0U) && (timeout_count < I2C_TIMEOUT_MAX)) {
			timeout_count++; // typical value seems to be < 3500
		}
		if (timeout_count >= I2C_TIMEOUT_MAX) {
			return I2C_TIMEOUT_FAIL;
		}

		// If a NACK occurred, SCL is held low and STP bit cleared: http://processors.wiki.ti.com/index.php/I2C_Tips
		if (i2c->STR & (uint32_t) I2C_NACK_INT) {
			i2cSetStop(i2cREG1);
			i2c->STR = I2C_NACKSNT; // write 1 to clear
			return I2C_ERR_NACK;
		}

		*data = ((uint8) i2c->DRR);
		data++;
		length--;
	}
	return I2C_OK;
}

/* Reset I2C
 *
 * - the bus can hang for many reasons. Our error handlers must therefore be able to reset the bus.
 * - To do this, we reset the I2C peripheral and clock out 10 pulses
 * - http://www.microchip.com/forums/m175368.aspx
 *
 * - Precondition: we should have the mutex
 *
 * - num_resets ensures we only attempt to reset 5 times before triggering the watchdog.
 * - if we reset successful, num_resets is set to 0 again.
 */
int16_t i2c_reset(i2cBASE_t *i2c){
	vTaskSuspendAll();							/* don't want any scheduling happening while we reset */
	do {
		i2c->MDR = (uint32)((uint32)0U << 5U); 	/* reset i2c peripheral */
		i2c->PFNC = (0x01);						/* I2C pins to GPIO mode */
		i2c->DIR = (0x01);						/* SDA is input [bit 1], SCL is output [bit 0], output = 1 */
		uint8_t i;

		/* send out some pulses */
		for(i = 0; i < 10; i++){
			i2c->DOUT = i2c->DOUT | 0x01; 		/* set SCL high */
			busy_wait(300);
			i2c->DOUT ^= i2c->DOUT; 			/* set SCL low */
			busy_wait(300);
		}
		num_resets++;

	} while (i2c->DIN & 0x02 != 0x02 && num_resets < 5);	/* check that the data line has gone high (idle) */

	if(num_resets >= 5) {
		serial_send_ln("I2C FAULT");
		while(1);								/* at this point, the watchdog will take care of us */
	}

    serial_send_ln("I2C RESET");
	i2c_init();
	xTaskResumeAll();
    return I2C_OK;
}
