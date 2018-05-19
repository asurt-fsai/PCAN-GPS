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
	SYSTIME_t timenow=0;	
	u8_t LED_toggleCAN=0;	//<! variable for LED toggle
	CANMsg_t  CycleMsg = {	//<! message that is transmitted cyclic
			.Id = 0x43C,
			.Len = 2,
			.Type = CAN_MSG_STANDARD,
			.Data.Data16[0] = 0
	};

	init_system();

	send_greeting();

	while(1) 	
	{
		CANMsg_t  RxMsg, TxMsg;
		SYSTIME_t  timenow;

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

		// copy message with id+1
		TxMsg.Id   = RxMsg.Id+1;
		TxMsg.Type = RxMsg.Type;
		TxMsg.Len  = RxMsg.Len;

		TxMsg.Data.Data32[0]	= RxMsg.Data.Data32[0];
		TxMsg.Data.Data32[1]	= RxMsg.Data.Data32[1];

		// send message back
		CAN_UserWrite (&TxMsg);
	}

	// send a message every 50 [ms]
	timenow = SYSTIME_NOW;

	if ( SYSTIME_DIFF (LastMsgTxTm, timenow) >= 50000)
	{
		// adjust last time of message by 50 millis
		LastMsgTxTm += 50000;

		// increment a simple counter
		CycleMsg.Data.Data16[0] += 1;

		// send message
		CAN_UserWrite (&CycleMsg);
		
	}
}
//---------------------------------------------------------------------



	
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

	HW_SetLED (HW_LED_STATUS_1, HW_LED_ORANGE);

	// Initialize CAN Controller
	CAN_UserInit();	
	
	HW_SetLED (HW_LED_STATUS_2, HW_LED_ORANGE);

	return;
}


//----------------------------------------------------------------------------
//! void send_greeting(void)
//----------------------------------------------------------------------------
//! @brief transmits "greeting" message at module start
//----------------------------------------------------------------------------
void send_greeting(void){
	CANMsg_t  Msg;

	Msg.Id   = 0x123;
	Msg.Len  = 8;
	Msg.Type = CAN_MSG_STANDARD;

	Msg.Data.Data32[0] = 0x67452301;
	Msg.Data.Data32[1] = 0xEFCDAB89;

	// Send Msg
	CAN_UserWrite (&Msg);
}


