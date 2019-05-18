/*
 * obc_temp.h
 *
 *  Created on: Feb 4, 2019
 *      Author: Melvin
 */

#ifndef OBC_TEMP_H_
#define OBC_TEMP_H_

#include "sys_common.h"

// Chip addresses for all sensors in the satellite
#define OBC_TEMP 0x48

int16_t obc_temp_test_no_rtos(uint8_t addr);
int16_t read_temp(uint8_t addr);			// suitable for RTOS use (includes mutex)
int16_t read_temp_raw(uint8_t addr);		// temp read without mutex

#endif /* OBC_TEMP_H_ */
