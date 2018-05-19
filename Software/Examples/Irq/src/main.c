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

u8_t process_acc_isr=0;
u8_t process_mag_isr=0;
u8_t process_gyro_isr=0;


/*******************************************************************************
 local definitions
 ******************************************************************************/
u8_t Initialized = 0;
u8_t persistant_config_used=0;


#define EEPROM_CFG_ADDR 0x00
S_CONFIG_DATA_t cfg_data;

#define INT_GYRO_0 	(1<<4)
#define INT_GYRO_1 	(1<<9)
#define INT_ACC1 	(1<<5)
#define INT_MAG 	(1<<6)
#define INT_1PPS 	(1<<29)

/*******************************************************************************
 local function prototypes
 ******************************************************************************/
static s32_t init_system(void);
static s32_t get_default_config(void);
void init_GPIO_interrupts(void) ;
static s32_t read_persistent_config(void);
void Timer_1000usec(void);
void send_greeting(void);


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
	u32_t mag_counter=0;

	Initialized = 0;

	init_system();

	send_greeting();

	while(1) 	
	{
		// handle data processing and sending outside of the real ISR

		if(process_gyro_isr){
			process_gyro_isr=0;
			MEMS_L3GD20_ReadData();
			CAN_UserSendL3GDData();
		}
		if(process_acc_isr){
			process_acc_isr=0;
			MEMS_BMC050_handleAccIRQ();
			CAN_UserSendBMCAccData();
		}
		if(process_mag_isr){
			process_mag_isr=0;
			MEMS_BMC050_handleMagIRQ();
		}

		// read new magnetic field data periodically cause no data ready irq is
		// available
		if (SYSTIME_DIFF(mag_counter, SYSTIME_NOW) >= 100000)
		{
			// update time interval
			mag_counter = SYSTIME_NOW;
			MEMS_BMC050_ReadMagData();
			CAN_UserSendBMCMagData();
		}

	}
}



	
//------------------------------------------------------------------------------
//! s32_t init_system(void)
//------------------------------------------------------------------------------
//! @brief initializes system to bring it into a fully functional state
//------------------------------------------------------------------------------
s32_t init_system(void){
	s32_t res = 0;

	// Initialize Basic Parts
	HW_Init();

	// Initialize Systemtimer
	Init_Timer0();

	// read configuration values from EEPROM if present

	init_GPIO_interrupts();

	res = read_persistent_config();
	if(res!=RET_OK)
		get_default_config();

	// Initialize CAN Controller
	CAN_UserInit();

	// Init GPS and UART2
	UBLOX_MAX7W_init();

	// Initialize gyroscope
	MEMS_L3GD20_init();

	//do some initial reads to clear first interrupts
	MEMS_L3GD20_ReadData();
	MEMS_L3GD20_ReadData();

	// Initialize magnetometer and accelerometer
	MEMS_BMC050_init_Accelerometer();
	MEMS_BMC050_init_Magnetometer();

	MEMS_BMC050_SetAccRange(cfg_data.Acc.range);

	MEMS_L3GD20_SetRange(cfg_data.Gyro.range);

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

//------------------------------------------------------------------------------
//! void send_greeting(void)
//------------------------------------------------------------------------------
//! @brief transmits "greeting" message at module start
//------------------------------------------------------------------------------
void send_greeting(void){
	CANMsg_t  Msg;

	Msg.Id   = 0x123;
	Msg.Len  = 8;
	Msg.Type = CAN_MSG_STANDARD;
	Msg.Data.Data32[0]=0;
	Msg.Data.Data32[1]=0;

	// we will read 8 bytes from eeprom address 100h
	EEPROM_Read (EEPROM_INT, 0x100, &Msg.Data.Data8[0], 8);

	// Send Msg
	CAN_UserWrite (&Msg);
}

//----------------------------------------------------------------------------
// void init_GPIO_interrupts(void) {
//----------------------------------------------------------------------------
//! @brief
//----------------------------------------------------------------------------
//! @param
//----------------------------------------------------------------------------
//! @return	tbd
//----------------------------------------------------------------------------
void init_GPIO_interrupts(void) {

	// disable all rising and falling edge interrupts for port 0
	LPC_GPIOINT->IO0IntEnF=INT_1PPS;
	LPC_GPIOINT->IO0IntEnR=INT_1PPS;
	//
	LPC_GPIOINT->IO2IntEnF=0;
	LPC_GPIOINT->IO2IntEnR= INT_GYRO_1 | INT_ACC1 | INT_MAG;
	// clear all interrupts
	LPC_GPIOINT->IO0IntClr = 0xFFFFFFFF;
	LPC_GPIOINT->IO2IntClr = 0xFFFFFFFF;

	NVIC_SetPriority (GPIO_IRQn, 10);
	NVIC_EnableIRQ (GPIO_IRQn);
}


//----------------------------------------------------------------------------
// void GPIO_IRQHandler(void)
//----------------------------------------------------------------------------
//! @brief
//----------------------------------------------------------------------------
//! @param
//----------------------------------------------------------------------------
//! @return	tbd
//----------------------------------------------------------------------------
void GPIO_IRQHandler(void) {
	
	// check for interrupts on GPIO Port 0
	if((LPC_GPIOINT->IntStatus & (1<<0)) == (1<<0)){
		if(LPC_GPIOINT->IO0IntStatR & (INT_1PPS)){
			HW_SetLED (HW_LED_STATUS_2, HW_LED_ORANGE);
			CAN_UserSend1PPS(1);
			LPC_GPIOINT->IO0IntClr = INT_1PPS;
		}
		if(LPC_GPIOINT->IO0IntStatF & (INT_1PPS)){
			HW_SetLED (HW_LED_STATUS_2, HW_LED_OFF);
			CAN_UserSend1PPS(0);
			LPC_GPIOINT->IO0IntClr = INT_1PPS;
		}
		LPC_GPIOINT->IO0IntClr = ~INT_1PPS;
	}

	// check for interrupts on GPIO Port 2
	if((LPC_GPIOINT->IntStatus & (1<<2)) == (1<<2)){
		if(LPC_GPIOINT->IO2IntStatR & (INT_GYRO_1)){
			process_gyro_isr = 1;
			LPC_GPIOINT->IO2IntClr = INT_GYRO_1;
		}
		if(LPC_GPIOINT->IO2IntStatR & (INT_ACC1)){
			process_acc_isr=1;
			LPC_GPIOINT->IO2IntClr = INT_ACC1;
		}
		if(LPC_GPIOINT->IO2IntStatR & (INT_MAG)){
			process_mag_isr=1;
			LPC_GPIOINT->IO2IntClr = INT_MAG;
		}
		// clear all other interrupts
		LPC_GPIOINT->IO2IntClr = ~INT_GYRO_1;
	}
}




