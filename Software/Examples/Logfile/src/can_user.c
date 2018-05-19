
//------------------------------------------------------------------------------
//
//	Module       : can_user.c
//
//  Project      : PCAN-GPS
//
//  Version/Date : 1.0 , 06/2014
//
//  Copyright (c): PEAK-SYSTEM TECHNIK GMBH, DARMSTADT
//
//------------------------------------------------------------------------------
/*******************************************************************************
 include files
 ******************************************************************************/
//
// System header files
//

//
// Library header files
//
#include <system_LPC407x_8x_177x_8x.h>
#include <lpc407x_8x_177x_8x.h>

//
// Source code header files
//
#include "typedefs.h"
#include "hardware.h"
#include "timer.h"
#include "MAX-7W.h"
#include "can.h"
#include "can_user.h"


/*******************************************************************************
 global definitions
 ******************************************************************************/


/*******************************************************************************
 local definitions
 ******************************************************************************/

#define SYM_GPS_STATUS			0x620
#define SYM_GPS_COURSE_SPEED	0x621
#define SYM_GPS_POS_LONGITUDE	0x622
#define SYM_GPS_POS_LATITUDE	0x623
#define SYM_GPS_POS_ALTITUDE	0x624
#define SYM_GPS_DELUSIONS_01	0x625
#define SYM_GPS_DELUSIONS_02	0x626
#define SYM_GPS_DATE_TIME		0x627


/*******************************************************************************
 local function prototypes
 ******************************************************************************/


// Queues for CAN interface
CANMsg_t TxQueueCAN1[CAN_TX_QUEUE_SIZE];
CANMsg_t RxQueueCAN1[CAN_RX_QUEUE_SIZE];

/*******************************************************************************
 global functions
 ******************************************************************************/


//------------------------------------------------------------------------------
//! CANStatus_t CAN_UserWrite(CANMsg_t *pBuff)
//------------------------------------------------------------------------------
//! @brief	Send a message on CAN bus
//------------------------------------------------------------------------------
CANStatus_t CAN_UserWrite(CANMsg_t *pBuff){
	CANStatus_t ret;
	CANMsg_t *pMsg;

	ret = CAN_ERR_OK;

	pMsg = CAN_TxQueueGetNext(CAN_HW_BUS2);

	if (pMsg != NULL) {
		pMsg->Id = pBuff->Id;
		pMsg->Len = pBuff->Len;
		pMsg->Type = pBuff->Type;

		pMsg->Data.Data32[0] = pBuff->Data.Data32[0];
		pMsg->Data.Data32[1] = pBuff->Data.Data32[1];

		// Send Msg
		ret = CAN_TxQueueWriteNext(CAN_HW_BUS2);
	} else {
		// Tx Queue FULL
		ret = CAN_ERR_FAIL;
	}

	return ret;
}

//------------------------------------------------------------------------------
//! u32_t CAN_UserRead(CANMsg_t *pBuff)
//------------------------------------------------------------------------------
//! @brief	Read message from CAN bus
//------------------------------------------------------------------------------
//! @return	 	1 is a message was read
//! 	 		0 otherwise
//------------------------------------------------------------------------------
u32_t CAN_UserRead(CANMsg_t *pBuff) {
	u32_t ret;
	CANMsg_t *pMsg;

	ret = 0;

	pMsg = CAN_RxQueueGetNext(CAN_HW_BUS2);

	if (pMsg != NULL) {
		pBuff->Id = pMsg->Id;
		pBuff->Len = pMsg->Len;
		pBuff->Type = pMsg->Type;

		pBuff->Data.Data32[0] = pMsg->Data.Data32[0];
		pBuff->Data.Data32[1] = pMsg->Data.Data32[1];

		CAN_RxQueueReadNext(CAN_HW_BUS2);
		ret = 1;
	}

	return ret;
}

//------------------------------------------------------------------------------
//! void CAN_UserInit(void)
//------------------------------------------------------------------------------
//! @brief	Initializes CAN bus
//------------------------------------------------------------------------------
void CAN_UserInit(void) {

	CAN_SetBusMode(CAN_HW_BUS2, BUS_OFF);					// CAN Bus On

	//! init CAN interface
	CAN_ReferenceTxQueue(CAN_HW_BUS2, &TxQueueCAN1[0], CAN_TX_QUEUE_SIZE);	// Reference above Arrays as Queues
	CAN_ReferenceRxQueue(CAN_HW_BUS2, &RxQueueCAN1[0], CAN_RX_QUEUE_SIZE);

	CAN_SetTimestampHandler(CAN_HW_BUS2, NULL);

	CAN_SetErrorLimit(CAN_HW_BUS2, STD_TX_ERRORLIMIT);

	CAN_SetTxErrorCallback(CAN_HW_BUS2, NULL);	// Set ErrorLimit & Callbacks
	CAN_SetRxCallback(CAN_HW_BUS2, NULL);

	CAN_SetChannelInfo(CAN_HW_BUS2, NULL);					// Textinfo is NULL

	// Setup Filters
	CAN_InitFilters();										// Clear Filter LUT
	CAN_SetFilterMode(AF_ON_BYPASS_ON);			// No Filters ( Bypassed)

	//! init CAN1 and CAN2 with Values above
	CAN_InitChannel(CAN_HW_BUS2, CAN_BAUD_500K);

	//! bring tranceivers into normal mode
	CAN_SetTransceiverMode(CAN_HW_BUS2, CAN_TRANSCEIVER_MODE_NORMAL);

	NVIC_SetPriority(CAN_IRQn, 0);
	NVIC_EnableIRQ(CAN_IRQn);

	//! Busses on
	CAN_SetBusMode(CAN_HW_BUS2, BUS_ON);					// CAN Bus On

	return;
}


//------------------------------------------------------------------------------
//! void CAN_UserSendGPSData(void)
//------------------------------------------------------------------------------
//! @brief	Sends latest GPS data
//------------------------------------------------------------------------------
void CAN_UserSendGPSData(void){
	CANMsg_t TxMsg;

	TxMsg.Id = SYM_GPS_STATUS;
	TxMsg.Len = 3;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.Data8[0] = MAX7W_Readings.Gps_AntennaStatus; // (0=INIT, 1=DONTKNOW, 2=OK, 3=SHORT, 4=OPEN)
	TxMsg.Data.Data8[1] = MAX7W_Readings.Nav_NumSatellites;
	TxMsg.Data.Data8[2] = MAX7W_Readings.Nav_Method;      	// None / 2D / 3D
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_COURSE_SPEED;
	TxMsg.Len = 8;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = MAX7W_Readings.Nav_CourseOverGround;
	TxMsg.Data.DataFlt[1] = MAX7W_Readings.Nav_SpeedOverGroundKmh;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_POS_LONGITUDE;
	TxMsg.Len = 5;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = MAX7W_Readings.Pos_Longitude;
	TxMsg.Data.Data8[4] = MAX7W_Readings.Pos_LongitudeIndEW;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_POS_LATITUDE;
	TxMsg.Len = 5;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = MAX7W_Readings.Pos_Latitude;
	TxMsg.Data.Data8[4] = MAX7W_Readings.Pos_LatitudeIndNS;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_POS_ALTITUDE;
	TxMsg.Len = 4;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = MAX7W_Readings.Pos_AltitudeOverSea;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_DELUSIONS_01;
	TxMsg.Len = 8;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = MAX7W_Readings.PDOP;
	TxMsg.Data.DataFlt[1] = MAX7W_Readings.HDOP;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_DELUSIONS_02;
	TxMsg.Len = 4;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = MAX7W_Readings.VDOP;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_GPS_DATE_TIME;
	TxMsg.Len = 6;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.Data8[0] = MAX7W_Readings.Date_Year;
	TxMsg.Data.Data8[1] = MAX7W_Readings.Date_Month;
	TxMsg.Data.Data8[2] = MAX7W_Readings.Date_DayOfMonth;
	TxMsg.Data.Data8[3] = MAX7W_Readings.Time_Hrs;
	TxMsg.Data.Data8[4] = MAX7W_Readings.Time_Min;
	TxMsg.Data.Data8[5] = MAX7W_Readings.Time_Sec;
	CAN_UserWrite(&TxMsg);

	return;
}


