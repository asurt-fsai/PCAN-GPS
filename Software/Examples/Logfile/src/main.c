/*******************************************************************************
 *
 * Project  :	PCAN-GPS - Timer Example
 * Module   :
 * Filename :	main.c
 * System   :
 * Compiler :
 * Switches :
 * Rights   : 	(c) PEAK-System Technik GmbH
 *            	www.peak-system.com
 *
 *******************************************************************************
 * Implementation description
 *
 *
 *
 *******************************************************************************
 * History: (newer entries first!)
 *------------------------------------------------------------------------------
 * Date / Name      Vers.   changes made
 *------------------------------------------------------------------------------
 * 2014 July StM	1.0.0	Initial Version
 ******************************************************************************/

/*******************************************************************************
 include files
 ******************************************************************************/
//
// System header files
//
#include <system_LPC407x_8x_177x_8x.h>
#include <lpc407x_8x_177x_8x.h>

//
// Library header files
//
#include "typedefs.h"
#include "hardware.h"
#include "ssp.h"
#include "can.h"
#include "eeprom.h"
#include "serial.h"
#include "crc.h"

//
// Source code header files
//
#include "timer.h"
#include "MEMS_BMC050.h"
#include "MEMS_L3GD20.h"
#include "MAX-7W.h"
#include "can_user.h"
#include "crc_data.h"
#include "rtc.h"
#include "log.h"


/*******************************************************************************
 global definitions
 ******************************************************************************/
//! identifier is needed by PCANFlash.exe -> do not delete
const b8_t Ident[] __attribute__ ((section(".ident"), used)) = { "PCAN-GPS"};

//! info data for PCANFlash.exe
const crc_array_t  C2F_Array __attribute__((section(".C2F_Info"), used)) = {
	.Str = CRC_IDENT_STRING,
	.Version = 0x21,
	.Day = 5,
	.Month = 5,
	.Year = 6,

	// crc infos are patched during link time by flash.ld
	// crc value is patched by PCANFlash.exe
};

#define EEPROM_CFG_ADDR 0x00
S_CONFIG_DATA_t cfg_data;

/*******************************************************************************
 local definitions
 ******************************************************************************/
u8_t Initialized = 0;



/*******************************************************************************
 local function prototypes
 ******************************************************************************/
static s32_t init_system(void);
void Timer_1000usec(void);


/*******************************************************************************
 global functions
 ******************************************************************************/

/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

DWORD get_fattime (void)
{
	RTC rtc;

	/* Get local time */
	rtc_gettime(&rtc);

	/* Pack date and time into a DWORD variable */
	return	  ((DWORD)(rtc.year - 1980) << 25)
			| ((DWORD)rtc.month << 21)
			| ((DWORD)rtc.mday << 16)
			| ((DWORD)rtc.hour << 11)
			| ((DWORD)rtc.min << 5)
			| ((DWORD)rtc.sec >> 1);
}


//------------------------------------------------------------------------------
//! void Timer_1000usec(void)
//------------------------------------------------------------------------------
//! @brief	this function is called every 1[ms] by timer0 match-interrupt ISR
//------------------------------------------------------------------------------
void Timer_1000usec(void){
	static u32_t  LedBlink_counter=0;
	static u8_t	toggle_led = 0;

	if (!Initialized)
		return;

	// call card control timing ever [ms]
	disk_timerproc();

	if (LedBlink_counter < 1000){
		LedBlink_counter++;
	}else{
		LedBlink_counter = 0;

		toggle_led ^= 1;		// invert flag and set new value
		if (toggle_led){
			HW_SetLED (HW_LED_STATUS_1, HW_LED_GREEN);
		} else {
			HW_SetLED(HW_LED_STATUS_1, HW_LED_OFF);
		}
	}
}


//------------------------------------------------------------------------------
//! int main(void)
//------------------------------------------------------------------------------
//! @brief	initialization and main loop
//------------------------------------------------------------------------------
int main(void)
{

	Initialized = 0;

	init_system();

	while(1) 	
	{
		static u32_t  		MAX7Wtask_counter=0;
		static u32_t  		LOGtask_counter=0;

		// evaluate new gps data as fast as possible
		UBLOX_MAX7W_task();
		if (SYSTIME_DIFF (MAX7Wtask_counter, SYSTIME_NOW) >= 100*1000)
		{
			// update time interval
			MAX7Wtask_counter = SYSTIME_NOW;
			CAN_UserSendGPSData();
		}

		if (SYSTIME_DIFF (LOGtask_counter, SYSTIME_NOW) >= 1*1000*1000)
		{
			// update time interval
			LOGtask_counter = SYSTIME_NOW;
			Log_task();
		}

	} // end while (1)
}



	
//------------------------------------------------------------------------------
//! s32_t init_system(void)
//------------------------------------------------------------------------------
//! @brief initializes system
//------------------------------------------------------------------------------
s32_t init_system(void){
	s32_t res = 0;

	// Initialize Basic Parts
	HW_Init();

	// Initialize Systemtimer
	Init_Timer0();

	rtc_initialize();

	// Initialize CAN Controller
	CAN_UserInit();

	// Init GPS
	UBLOX_MAX7W_init();

	Initialized = 1;
	return RET_OK;
}


