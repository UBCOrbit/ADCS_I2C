/*
 * temp_stts75.h
 *
 *  Created on: Feb 4, 2019
 *      Author: Melvin
 */
#ifndef ORCASAT_HARDWARE_DRIVERS_TEMP_STTS75_H_
#define ORCASAT_HARDWARE_DRIVERS_TEMP_STTS75_H_

#include "sys_common.h"
#include <stdint.h>
#include <math.h>

// CHIP COMMANDS
#define RD_CMD 0x1
#define WR_CMD 0x0


// DEVICE ADDRESS
#define STTS75_ADDR 0x48

/*
 * Registers
 */
#define STTS75_TEMPERATURE                  0x00U
#define STTS75_CONFIGURATION                0x01U
#define STTS75_HYSTERESIS                  0x02U
#define STTS75_OVERTEMP                    0x03U

#define TEMP_READ_ERROR -99
#define TEMP_RES_GET_ERROR -98
#define TEMP_FAULT_GET_ERROR -97
#define TEMP_MODE_GET_ERROR -96
#define TEMP_ALARM_GET_ERROR -95

typedef struct {
  uint8_t shutdown       : 1;
  uint8_t output_polarity: 1;
  uint8_t fault          : 2;
  uint8_t resolution     : 2;
  uint8_t one_shot       : 1;
  uint8_t thermostat     : 1;
} stts75_configuration_t;

typedef enum {
  STTS75_9bit      = 0,
  STTS75_10bit     = 1,
  STTS75_11bit     = 2,
  STTS75_12bit     = 3,
} stts75_resolution_t;

typedef enum {
  one_fault     = 0,
  two_fault     = 1,
  four_fault    = 2,
  six_fault     = 3,
} stts75_fault_t;

typedef enum {
  continuous_conversion    = 0,
  shutdown                 = 1,
  one_shot                 = 3,
} stts75_mode_t;

typedef enum {
  comparator    = 0,
  interrupt_m     = 1,
} stts75_thermostat_t;

int8_t stts75_set_read_write(uint8_t reg, boolean flag); // true = read, false = write
int8_t stts75_read_reg(uint8_t reg, uint8_t* data, uint16_t len);
int8_t stts75_write_reg(uint8_t reg, uint8_t* data, uint16_t len);

// suitable for RTOS use (includes mutex)
int16_t stts75_read_temp(uint8_t addr);

int8_t stts75_hysteresis_set(int16_t val);
int8_t stts75_hysteresis_get(int16_t* val);

int8_t stts75_overtemp_set(int16_t val);
int8_t stts75_overtemp_get(int16_t* val);

int8_t stts75_mode_set(stts75_mode_t val);
int8_t stts75_mode_get(stts75_mode_t *val);

int8_t stts75_alarm_mode_set(stts75_thermostat_t val);
int8_t stts75_alarm_mode_get(stts75_thermostat_t *val);

int8_t stts75_resolution_set(stts75_resolution_t val);
int8_t stts75_resolution_get(stts75_resolution_t *val);

int8_t stts75_fault_set(stts75_fault_t val);
int8_t stts75_fault_get(stts75_fault_t *val);

// temp read without mutex
int16_t stts75_obc_temp_test_no_rtos(uint8_t addr);

int16_t stts75_read_temp_raw(uint8_t addr);

#endif /* ORCASAT_HARDWARE_DRIVERS_TEMP_STTS75_H_ */
