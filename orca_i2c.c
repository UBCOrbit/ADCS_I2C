#include "obc_i2c.h"

#define RD_CMD 0x1
#define WR_CMD 0x0

void prvConfigureSend(uint8_t addr, uint32 count){
	i2cSetSlaveAdd(i2cREG1, addr);
	i2cSetDirection(i2cREG1, I2C_TRANSMITTER);
	i2cSetCount(i2cREG1, count);
	i2cSetMode(i2cREG1, I2C_MASTER);
}

void prvConfigureRecieve(uint8_t addr, uint32 count){
	i2cSetSlaveAdd(i2cREG1, addr);
	i2cSetCount(i2cREG1, count);
	i2cSetDirection(i2cREG1, I2C_RECEIVER);
	i2cSetMode(i2cREG1, I2C_MASTER);
}

int16_t check_other_error_conditions(){
	int16_t errcode;
	errcode = i2c_is_bus_busy();
	if (errcode != I2C_OK)
		return errcode;

	errcode = i2c_is_ok_transmit();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int16_t select_register(uint8_t addr, uint8_t reg){
	uint8_t cmd = (addr << 1 | WR_CMD);
	uint8_t data[2] = {cmd, reg}; // slave address | write bit | register

	// Start the transaction
	prvConfigureSend(addr, 2);

	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);

	int16_t errcode = i2c_send(i2cREG1, 2, data);
	if (errcode != I2C_OK)
		return errcode;

	// Complete the transaction
	i2cSetStop(i2cREG1);
	i2cClearSCD(i2cREG1);

	// Check other error conditions
	errcode = check_other_error_conditions();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int16_t read_n_bytes(uint8_t addr, uint32 len, uint8_t* result){
	uint8_t cmd = (addr << 1 | RD_CMD); // slave address | read bit
	int16_t errcode = 0;

	// Select the slave
	prvConfigureSend(addr, 2);

	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);

	errcode = i2c_send(i2cREG1, 1, &cmd);
	if (errcode != I2C_OK)
		return errcode;

	// Read n bytes
	prvConfigureRecieve(addr, len);

	errcode = i2c_receive(i2cREG1, len, result);
	if (errcode != I2C_OK)
		return errcode;

	i2cSetStop(i2cREG1);
	i2cClearSCD(i2cREG1);

	// Check other error conditions
	errcode = check_other_error_conditions();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int16_t write_one_byte(uint8_t addr, uint8_t byte){
	uint8_t cmd = (addr << 1 | WR_CMD);
	uint8_t data[2] = {cmd, byte}; // slave address | write bit | Byte

	// Start the transaction
	prvConfigureSend(addr, 2);

	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);

	int16_t errcode = i2c_send(i2cREG1, 2, data);
	if (errcode != I2C_OK)
		return errcode;

	// Complete the transaction
	i2cSetStop(i2cREG1);
	i2cClearSCD(i2cREG1);

	// Check other error conditions
	errcode = check_other_error_conditions();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}

int16_t write_two_bytes(uint8_t addr, uint8_t bytes[2]){
	uint8_t cmd = (addr << 1 | WR_CMD);
	uint8_t data[3] = {cmd, bytes[0], bytes[1]}; // slave address | write bit | MSB | LSB

	// Start the transaction
	prvConfigureSend(addr, 3);

	i2cSetStop(i2cREG1);
	i2cSetStart(i2cREG1);

	int16_t errcode = i2c_send(i2cREG1, 3, data);
	if (errcode != I2C_OK)
		return errcode;

	// Complete the transaction
	i2cSetStop(i2cREG1);
	i2cClearSCD(i2cREG1);

	// Check other error conditions
	errcode = check_other_error_conditions();
	if (errcode != I2C_OK)
		return errcode;

	return I2C_OK;
}
