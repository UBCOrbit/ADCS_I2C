/*
 * temp_stts75.c
 *
 *  Created on: Feb 4, 2019
 *      Author: Melvin
 */

#include "temp_stts75.h"
#include "FreeRTOS.h"
#include "rtos_semphr.h"
#include "i2c.h"
#include "reg_i2c.h"
#include "obc_i2c.h"
#include "obc_uart.h"

/* Read/Write
 * - These functions allow for read and write capabilites to a generic register over I2C
 * - Inputs: Register, data to write/store, length of reading/writing
 * - Ouputs: Returns OK if succesful, errorcode if not
 */

int8_t stts75_set_read_write(uint8_t addr, boolean flag) {

	int8_t errcode = 0;
	uint8_t cmd = 0;

	if (flag){
		cmd = (addr << 1 | RD_CMD);
	}
	else{
		cmd = (addr << 1 | WR_CMD);
	}

	i2cSetSlaveAdd(i2cREG1, addr);
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);
	i2cSetCount(i2cREG1, 1); 							// the total number of bytes to transact before sending stop bit
	i2cSetMode(i2cREG1, I2C_MASTER);
	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);
	errcode = i2c_send(i2cREG1, 1, &cmd);
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int8_t stts75_read_reg(uint8_t reg, uint8_t* data, uint16_t len) {
	stts75_set_read_write(reg, true);
	i2cSetSlaveAdd(i2cREG1, reg);
	i2cSetCount(i2cREG1, len); 							// the total number of bytes to transact before sending stop bit
	i2cSetDirection(i2cREG1, I2C_RECEIVER);
	i2cSetMode(i2cREG1, I2C_MASTER);
	int32_t errcode = i2c_receive(i2cREG1, len, data);
	if (errcode != I2C_OK)
		return errcode;
	i2cSetStop(i2cREG1);
	i2cClearSCD(i2cREG1);
	
	errcode = i2c_is_bus_busy();
	if (errcode != I2C_OK)
		return errcode;

	errcode = i2c_is_ok_transmit();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int8_t stts75_write_reg(uint8_t reg, uint8_t* data, uint16_t len) {
	stts75_set_read_write(reg, false);
	i2cSetSlaveAdd(i2cREG1, reg);
	i2cSetCount(i2cREG1, len); 							// the total number of bytes to transact before sending stop bit
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);
	i2cSetMode(i2cREG1, I2C_MASTER);
	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);
	int32_t errcode = i2c_send(i2cREG1, len, data);
	if (errcode != I2C_OK)
		return errcode;
	i2cSetStop(i2cREG1);
	i2cClearSCD(i2cREG1);

	errcode = i2c_is_bus_busy();
	if (errcode != I2C_OK)
		return errcode;

	errcode = i2c_is_ok_transmit();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

/* Read_temp
 * 	- this is the main temperature reading function
 * 	- pass it the correct temp sensor address (in stlm75.h)
 * 	- be sure to check for values -999 and less in the calling function
 * 	- largely negative results indicate a particular error
 */

int16_t stts75_read_temp(uint8_t addr) {
	if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(500) ) == pdTRUE) {
		int16_t temp;
		temp = stts75_read_temp_raw(addr);
		if (temp <= -20000) {
//			sfu_reset_i2c(i2cREG1);
		}
		xSemaphoreGive(xI2CMutex);
		return temp;
	} else {
		serial_send_ln("Temp read can't get mutex");

//		xSemaphoreGive(xI2CMutex);
		return TEMP_READ_ERROR;
	}

}

/* Hysteresis
 * - These functions set and get the hysterisis temperature stored in the hysterisis register of the sensor (2 bytes)
 * - This temperature acts as the minimum temperature when triggering faults
 * - Inputs: Desired hysterisis temp/pointer to store current hysterisis temp
 * - Ouputs: Returns OK if succesful, errorcode if not
 */

int8_t stts75_hysteresis_set(int16_t val) {
	int8_t ret;

	ret = stts75_write_reg(STTS75_HYSTERESIS, (uint8_t*)&val, 2);
	return ret;
}

int8_t stts75_hysteresis_get(int16_t* val) {
	int8_t ret;

	ret = stts75_read_reg(STTS75_HYSTERESIS, (uint8_t*)val, 2);
	return ret;
}

/* Overlimit Temperature
 * - These functions set and get the overlimit temperature stored in the overlimit register of the sensor (2 bytes)
 * - This temperature acts as the maximum temperature when triggering faults
 * - Inputs: Desired overlimit temp/pointer to store current overlimit temp
 * - Ouputs: Returns OK if succesful, errorcode if not
 */

int8_t stts75_overtemp_set(int16_t val) {
	int8_t ret;

	ret = stts75_write_reg(STTS75_OVERTEMP, (uint8_t*)&val, 2);
	return ret;
}

int8_t stts75_overtemp_get(int16_t* val) {
	int8_t ret;

	ret = stts75_read_reg(STTS75_OVERTEMP, (uint8_t*)val, 2);
	return ret;
}

/* Mode
 * - These functions set and get the mode the sensor is currently in by manipulating the configuration register of the sensor (1 byte)
 * - continuous conversion, shutdown and one-shot are the multiple modes available
 * - Inputs: Desired mode/pointer to store current mode
 * - Ouputs: Returns OK if succesful, errorcode if not
 */

int8_t stts75_mode_set(stts75_mode_t val) {
	stts75_configuration_t reg;
	int8_t ret;

	ret = stts75_read_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);

	switch (val) {
		case continuous_conversion:
			reg.one_shot = 0;
			reg.shutdown = 0;
			break;
		case shutdown:
			reg.one_shot = 0;
			reg.shutdown = 1;
			break;
		case one_shot:
			reg.one_shot = 1;
			reg.shutdown = 0;
			break;
		default:
			reg.one_shot = 0;
			reg.shutdown = 0;
			break;
	}

	ret = stts75_write_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);

	return ret;
}

int8_t stts75_mode_get(stts75_mode_t* val) {
	stts75_configuration_t reg;
	int8_t ret;

	ret = stts75_read_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);

	if (reg.one_shot == 0 && reg.shutdown == 0){
		*val = continuous_conversion;
	}
	else if (reg.one_shot == 0 && reg.shutdown == 1){
		*val = shutdown;
	}
	else if (reg.one_shot == 0 && reg.shutdown == 0){
		*val = one_shot;
	}
	else{
		return TEMP_MODE_GET_ERROR;
	}

	return ret;
}

/* Resolution
 * - These functions set and get the variable resolution of the sensor by manipulating the configuration register of the sensor (1 byte)
 * - 9bit, 10bit, 11bit, and 12bit resolutions available
 * - Inputs: Desired resolution/pointer to store current resolution
 * - Ouputs: Returns OK if succesful, errorcode if not
 */

int8_t stts75_resolution_set(stts75_resolution_t val) {
	stts75_configuration_t reg;
	int8_t ret;

	ret = stts75_read_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);

	if (ret == I2C_OK) {
		reg.resolution = (uint8_t) val;
		ret = stts75_write_reg(STTS75_CONFIGURATION,(uint8_t*) &reg, 1);
	}
	
	return ret;
}

int8_t stts75_resolution_get(stts75_resolution_t* val) {
	stts75_configuration_t reg;
	int8_t ret;

	ret = stts75_read_reg(STTS75_CONFIGURATION,(uint8_t*) &reg, 1);

	switch (reg.resolution) {
		case STTS75_9bit:
			*val = STTS75_9bit;
			break;
		case STTS75_10bit:
			*val = STTS75_10bit;
			break;
		case STTS75_11bit:
			*val = STTS75_11bit;
			break;
		case STTS75_12bit:
			*val = STTS75_12bit;
			break;
		default:
			return TEMP_RES_GET_ERROR;
	}
	return ret;
}

/* Fault
 * - These functions set and get the number of faults allowed before activating the OS/INT pin by manipulating the configuration register of the sensor (1 byte)
 * - 1, 2, 4, and 6 faults available
 * - Inputs: Desired fault/pointer to store current fault
 * - Ouputs: Returns OK if succesful, errorcode if not
 */

int8_t stts75_fault_set(stts75_fault_t val) {
	stts75_configuration_t reg;
	int8_t ret;

	ret = stts75_read_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);

	if (ret == I2C_OK) {
		reg.fault = (uint8_t) val;
		ret = stts75_write_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);
	}
	
	return ret;
}

int8_t stts75_fault_get(stts75_fault_t* val) {
	
	stts75_configuration_t reg;
	int8_t ret;

	ret = stts75_read_reg(STTS75_CONFIGURATION, (uint8_t*) &reg, 1);

	switch (reg.fault) {
		case one_fault:
			*val = one_fault;
			break;
		case two_fault:
			*val = two_fault;
			break;
		case four_fault:
			*val = four_fault;
			break;
		case six_fault:
			*val = six_fault;
			break;
		default:
			return TEMP_FAULT_GET_ERROR;
	}
	return ret;
}

/* Read temperature
 * - from page 24 of datasheet:
 * - Send out: [address 7:1][0], [0] means read
 * - STLM 75 will return two bytes, left aligned
 * - details about scaling were taken from here: https://github.com/jfitter/LM75/blob/master/LM75.cpp
 *
 * - Will return an I2C error code if one happens.
 * - Calling function is responsible for handling the error (logging, retrying, etc.).
 */
int16_t stts75_read_temp_raw(uint8_t addr) {
	uint8_t data[2] = { '\0' };
	uint8_t *dataval = data;
	uint8_t cmd = (addr << 1 | RD_CMD);
	int16_t temp_deg_c = 0;
	stts75_resolution_t resolution;
	int16_t ret;

	ret = stts75_write_reg(addr, &cmd, 1);

	ret = stts75_read_reg(addr, data, 2);

	temp_deg_c = *dataval << 8; 										// MSBits
	temp_deg_c = temp_deg_c | *(dataval+1); 							// LSBits

	ret =  stts75_resolution_get(&resolution);

	if (ret == I2C_OK){
		switch (resolution) {
			case STTS75_9bit:
				temp_deg_c = (temp_deg_c & 0xFFF0) * 0.5/128.0;
				break;
			case STTS75_10bit:
				temp_deg_c = (temp_deg_c & 0xFFF0) * 0.25/64.0;
				break;
			case STTS75_11bit:
				temp_deg_c = (temp_deg_c & 0xFFF0) * 0.125/32.0;
				break;
			case STTS75_12bit:
				temp_deg_c = (temp_deg_c & 0xFFF0) * 0.0625/16.0;
				break;
			default:
				return TEMP_READ_ERROR;
		}
	}
	else{
		return ret;
	}

	return temp_deg_c;
}

/* sanity check that the OBC temp sensor returns something
 * - not for flight
 * */
int16_t stts75_obc_temp_test_no_rtos(uint8_t addr) {
	int16_t temp;
	temp = stts75_read_temp_raw(addr);
	if (temp <= -20000) {
		i2c_reset(i2cREG1);
	}
	return temp;
}

void stts75_simple_test() {
	serial_send_ln("***\n\rSTTS75 test:");
	int16_t temperature = stts75_read_temp_raw(0x48);
	char buffer_temp[35] = {0};
	if (temperature <= -20000) {
		snprintf(buffer_temp, 35, "STTS75 Read Error: %d", temperature);
	} else {
		snprintf(buffer_temp, 35, "STTS75 Temperature: %d", temperature);
	}
	serial_send_ln(buffer_temp);
	serial_send_ln("STTS75 test complete\n\r***\n\r");
}
