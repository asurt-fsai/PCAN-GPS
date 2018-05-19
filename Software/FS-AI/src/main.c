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
 * 2014 October StM	1.0.2	Changed wrong mask when sending module ID
 * 2014 October StM	1.0.1	Changed format for GPS position in CAN frame
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
#include "can.h"
#include "ssp.h"
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
u8_t persistant_config_used=0;



/*******************************************************************************
 local function prototypes
 ******************************************************************************/
static s32_t init_system(void);
static s32_t get_default_config(void);
static s32_t read_persistent_config(void);
void Timer_1000usec(void);


/*******************************************************************************
 global functions
 ******************************************************************************/



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
		static u32_t  		BMC050task_counter=0;
		static u32_t  		L3GD20task_counter=0;
		static u32_t  		MAX7Wtask_counter=0;
		static u32_t  		DIO_counter=0;
		static u32_t  		TIME_counter=0;

		// read new compass/accelerometer/temperature data
		if (SYSTIME_DIFF(BMC050task_counter, SYSTIME_NOW) >= 25000)
		{
			// update time interval
			BMC050task_counter = SYSTIME_NOW;
			// as long as there is not a complete set of readings
			if (BMC050_Readings.data_valid == FALSE)
			{
				MEMS_BMC050_task();
			
				if (BMC050_Readings.data_valid == TRUE){
					CAN_UserSendBMCData();
				}
			}
		}
		//-------------------------
		
		// read new gyro/temperature data
		if (SYSTIME_DIFF (L3GD20task_counter, SYSTIME_NOW) >= 50000)
		{
			// update time interval
			L3GD20task_counter = SYSTIME_NOW;
			// as long as there is not a complete set of readings
			MEMS_L3GD20_task();
			
			if (L3GD20_Readings.data_valid == TRUE)
				CAN_UserSendL3GDData();
		}
		//-------------------------

		// evaluate new gps data as fast as possible
		UBLOX_MAX7W_task();
		if (SYSTIME_DIFF (MAX7Wtask_counter, SYSTIME_NOW) >= 100000)
		{
			// update time interval
			MAX7Wtask_counter = SYSTIME_NOW;
			CAN_UserSendGPSData();
		}
		//-------------------------
		if (SYSTIME_DIFF (DIO_counter, SYSTIME_NOW) >= 25000)
		{
			// update time interval
			DIO_counter = SYSTIME_NOW;
			CAN_UserSendDioData();
		}
		//-------------------------

		if (SYSTIME_DIFF (TIME_counter, SYSTIME_NOW) >= 500000)
		{
			// update time interval
			TIME_counter = SYSTIME_NOW;
			CAN_UserSendRTCTime();
		}
		//-------------------------

		CAN_UserProcessMsg();
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

	// read configuration values from EEPROM if present
	res = read_persistent_config();
	if(res!=RET_OK)
		get_default_config();

	// Initialize CAN Controller
	CAN_UserInit();

	// Init GPS and UART2
	UBLOX_MAX7W_init();

	// Initialize gyroscope
	MEMS_L3GD20_init();

	// Initialize magnetometer and accelerometer
	MEMS_BMC050_init_Accelerometer();
	MEMS_BMC050_init_Magnetometer();

	// set values read from EEPROM *after* initialization of sensors with
	// default values
	MEMS_BMC050_SetAccCalTargets(	&cfg_data.Acc.cmp_target_x,
									&cfg_data.Acc.cmp_target_y,
									&cfg_data.Acc.cmp_target_z);
	MEMS_BMC050_SetAccRange(cfg_data.Acc.range);

	MEMS_L3GD20_SetRange(cfg_data.Gyro.range);

	// Transmit initial configuration values once
	CAN_UserResetPanelValues();

	Initialized = 1;
	return RET_OK;
}



//------------------------------------------------------------------------------
//! static s32_t get_default_config(void)
//------------------------------------------------------------------------------
//! @brief	sets system configuration to default values
//------------------------------------------------------------------------------
static s32_t get_default_config(void){

	cfg_data.Acc.cmp_target_x = 0;
	cfg_data.Acc.cmp_target_y = 0;
	cfg_data.Acc.cmp_target_z = 0;	
	cfg_data.Acc.cmp_filt_x = 0;
	cfg_data.Acc.cmp_filt_y = 0;
	cfg_data.Acc.cmp_filt_z = 0;
	cfg_data.Acc.cmp_raw_x = 0;
	cfg_data.Acc.cmp_raw_y = 0;
	cfg_data.Acc.cmp_raw_z = 0;
	cfg_data.Acc.flags = 0;
	
	cfg_data.Acc.range = 1;

	cfg_data.Gyro.range = 0;
	
	return RET_OK;
}



//------------------------------------------------------------------------------
//! static s32_t read_persistent_config(void)
//------------------------------------------------------------------------------
//! @brief reads configuration from EEPROM
//------------------------------------------------------------------------------
//! @return	RET_ERR if EEPROM read fails or if CRC checksum was not valid
//!			RET_OK	elsewise
//------------------------------------------------------------------------------
static s32_t read_persistent_config(void){
	CRCInit_t cfg= CRC32_CONFIG;
	u32_t res=0;
	u32_t cnt=0;
	
	//! read static configuration data from eeprom
	if(EEPROM_Read(EEPROM_INT, EEPROM_CFG_ADDR,
			(u8_t*)&cfg_data, sizeof(cfg_data))!=EEPROM_ERR_OK)
		return RET_ERR;

	cnt = sizeof(cfg_data)-sizeofmember(S_CONFIG_DATA_t, crc32);
	res = CRC_Valid(&cfg_data, cnt, t_crc_8_bit, &cfg, cfg_data.crc32);
	if(res!=1)
		return RET_ERR;

	persistant_config_used = 1;

	return RET_OK;
}


