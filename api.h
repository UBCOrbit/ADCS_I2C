#ifndef ORCA_I2C_H_
#define ORCA_I2C_H_

#include "sys_common.h"

void prvConfigureSend(uint8_t addr, uint32 count);
void prvConfigureRecieve(uint8_t addr, uint32 count);
int16_t check_other_error_conditions();
int16_t select_register(uint8_t addr, uint8_t reg);
int16_t read_n_bytes(uint8_t addr, uint32 len, uint8_t* result);
int16_t write_one_byte(uint8_t addr, uint8_t byte);
int16_t write_two_bytes(uint8_t addr, uint8_t bytes[2]);

#endif /* ORCA_I2C_H_ */
