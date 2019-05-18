/*
 * obc_temp.c
 *
 *  Created on: Feb 4, 2019
 *      Author: Melvin Matthews
 */

#include "obc_temp.h"
#include "sys_common.h"
#include "temp_stts75.h"
#include "temp_stlm75.h"

/* Read_temp
 * 	- this is the main temperature reading function
 * 	- pass it the correct temp sensor address (in stlm75.h)
 * 	- be sure to check for values -999 and less in the calling function
 * 	- largely negative results indicate a particular error
 */

int16_t read_temp(uint8_t addr) {
	#ifdef PLATFORM_ORCA_V1
		return stts75_read_temp(addr);  // call appropriate function for new hardware
	#endif /* PLATFORM_ORCA_V1 */

	#ifdef PLATFORM_OBC_V0_4
		return stlm75_read_temp(addr);   // call legacy function for old hardware
	#endif /* PLATFORM_OBC_V0_4 */

	#ifdef PLATFORM_LAUNCHPAD_1224
		return stlm75_read_temp(addr);   // call legacy function for old hardware
	#endif /* PLATFORM_LAUNCHPAD_1224 */
}

/* sanity check that the OBC temp sensor returns something
 * - not for flight
 * */
int16_t obc_temp_test_no_rtos(uint8_t addr) {
	#ifdef PLATFORM_ORCA_V1
		return stts75_obc_temp_test_no_rtos(addr);  // call appropriate function for new hardware
	#endif /* PLATFORM_ORCA_V1 */

	#ifdef PLATFORM_OBC_V0_4
		return stlm75_obc_temp_test_no_rtos(addr);   // call legacy function for old hardware
	#endif /* PLATFORM_OBC_V0_4 */

	#ifdef PLATFORM_LAUNCHPAD_1224
		return stlm75_obc_temp_test_no_rtos(addr);   // call legacy function for old hardware
	#endif /* PLATFORM_LAUNCHPAD_1224 */
}

