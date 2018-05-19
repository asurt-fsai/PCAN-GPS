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
#include "eeprom.h"
#include "can.h"


//
// Source code header files
//
#include "timer.h"
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


/*******************************************************************************
 local definitions
 ******************************************************************************/


/*******************************************************************************
 local function prototypes
 ******************************************************************************/
void init_system(void);
void send_greeting(void);



/*******************************************************************************
 global functions
 ******************************************************************************/


//------------------------------------------------------------------------------
//! int main(void)
//------------------------------------------------------------------------------
//! @brief	initialization and main loop
//------------------------------------------------------------------------------
int main(void)
{
	SYSTIME_t LastMsgTxTm=0;	//<! variable to store time of last cyclic message transmission
	u8_t LED_toggleCAN=0;	//<! variable for LED toggle

	init_system();

	send_greeting();

	while(1) 	
	{
		CANMsg_t  RxMsg, TxMsg;

		// process messages from CAN bus
		if (CAN_UserRead(&RxMsg) == 0)
			continue;

		// message received
		LED_toggleCAN ^= 1;
		if (LED_toggleCAN) {
			HW_SetLED(HW_LED_STATUS_1, HW_LED_ORANGE);
		} else {
			HW_SetLED(HW_LED_STATUS_1, HW_LED_GREEN);
		}

		// catch ID 54Ah to involve the CAN2Flash Bootloader
		if(RxMsg.Id==0x54A && RxMsg.Type==CAN_MSG_STANDARD)	{
			// check the magic numbers from byte 0 and 1
			if ( RxMsg.Data.Data16[0] == 0xAA55)
			{
				// Jump to the bootloader and pass a user specific baudrate. Send a message like:
				// 54Ah		8		55h AAh 00h 00h 00h 00h 00h 00h
				// to start bootloader with default baudrate, or use:
				// 54Ah		8		55h AAh 00h 00h 0Fh 00h 2Ah 00h
				// to start bootloader at CAN_BAUD_250K. See can_user.h file for more baudrates.
				HW_JumpToBootloader ( RxMsg.Data.Data32[1]);
			}
		} else {
			// copy message
			TxMsg.Id   = RxMsg.Id+1;
			TxMsg.Type = RxMsg.Type;
			TxMsg.Len  = RxMsg.Len;

			TxMsg.Data.Data32[0]	= RxMsg.Data.Data32[0];
			TxMsg.Data.Data32[1]	= RxMsg.Data.Data32[1];

			// send
			CAN_UserWrite(&TxMsg);
		}
	}
}



	
//------------------------------------------------------------------------------
//! void init_system(void)
//------------------------------------------------------------------------------
//! @brief initializes system to bring it into a fully functional state
//------------------------------------------------------------------------------
void init_system(void){

	// Initialize Basic Parts
	HW_Init();

	// Initialize Systemtimer
	Init_Timer0();

	// Initialize CAN Controller
	CAN_UserInit();	
	
	HW_SetLED(HW_LED_STATUS_1, HW_LED_GREEN);
	HW_SetLED(HW_LED_STATUS_2, HW_LED_GREEN);

	return;
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

	Msg.Data.Data32[0] = 0x67452301;
	Msg.Data.Data32[1] = 0xEFCDAB89;

	// we will read 8 bytes from eeprom address 100h
	EEPROM_Read (EEPROM_INT, 0x100, &Msg.Data.Data8[0], 8);

	// Send Msg
	CAN_UserWrite(&Msg);
}


