
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
#include "ssp.h"
#include "MEMS_BMC050.h"
#include "MEMS_L3GD20.h"
#include "MAX-7W.h"
#include "can.h"
#include "can_user.h"
#include "eeprom.h"
#include "crc.h"
#include "crc_data.h"


/*******************************************************************************
 global definitions
 ******************************************************************************/


/*******************************************************************************
 local definitions
 ******************************************************************************/

#define SYM_BMC_ACCELERATION	0x600
#define SYM_BMC_MAGNETIC_FIELD	0x601

#define SYM_L3GD_ROTATION_01	0x610
#define SYM_L3GD_ROTATION_02	0x611

#define SYM_GPS_1PPS			0x628


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
//! void CAN_UserSendBMCAccData(void)
//------------------------------------------------------------------------------
//! @brief	Sends last read data of the BMC050 sensor
//------------------------------------------------------------------------------
void CAN_UserSendBMCAccData(void){
	CANMsg_t TxMsg;
	u8_t tmp8u=0;

	TxMsg.Id  = SYM_BMC_ACCELERATION;
	TxMsg.Len = 8;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.Data16[0] = BMC050_Readings.Acceleration_X;
	TxMsg.Data.Data16[1] = BMC050_Readings.Acceleration_Y;
	TxMsg.Data.Data16[2] = BMC050_Readings.Acceleration_Z;
	TxMsg.Data.Data8[6] = BMC050_Readings.Temperature;
	MEMS_BMC050_GetVertialAxis(&tmp8u);
	TxMsg.Data.Data8[7] = (tmp8u&0x3) | (BMC050_Readings.orientation&0x7)<<2;
	CAN_UserWrite(&TxMsg);

	return;
}


//------------------------------------------------------------------------------
//! void CAN_UserSendBMCMagData(void)
//------------------------------------------------------------------------------
//! @brief	Sends last read data of the BMC050 sensor
//------------------------------------------------------------------------------
void CAN_UserSendBMCMagData(void){
	CANMsg_t TxMsg;
	u8_t tmp8u=0;

	TxMsg.Id  = SYM_BMC_MAGNETIC_FIELD;
	TxMsg.Len = 6;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.Data16[0] = BMC050_Readings.MagField_X;
	TxMsg.Data.Data16[1] = BMC050_Readings.MagField_Y;
	TxMsg.Data.Data16[2] = BMC050_Readings.MagField_Z;
	CAN_UserWrite(&TxMsg);

	return;
}

//------------------------------------------------------------------------------
//! void CAN_UserSendL3GDData(void)
//------------------------------------------------------------------------------
//! @brief	Sends last read data of the L3GD20 sensor
//------------------------------------------------------------------------------
void CAN_UserSendL3GDData(void){
	CANMsg_t TxMsg;

	TxMsg.Id = SYM_L3GD_ROTATION_01;
	TxMsg.Len = 8;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = L3GD20_Readings.Gyro_X;
	TxMsg.Data.DataFlt[1] = L3GD20_Readings.Gyro_Y;
	CAN_UserWrite(&TxMsg);

	TxMsg.Id = SYM_L3GD_ROTATION_02;
	TxMsg.Len = 4;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.DataFlt[0] = L3GD20_Readings.Gyro_Z;
	CAN_UserWrite(&TxMsg);

	return;
}

//------------------------------------------------------------------------------
//! void CAN_UserSend1PPS(u8_t val)
//------------------------------------------------------------------------------
//! @brief	Sends GPS time pulse message
//------------------------------------------------------------------------------
void CAN_UserSend1PPS(u8_t val){
	CANMsg_t TxMsg;

	TxMsg.Id = SYM_GPS_1PPS;
	TxMsg.Len = 1;
	TxMsg.Type = CAN_MSG_STANDARD;
	TxMsg.Data.Data8[0]=val;
	CAN_UserWrite(&TxMsg);

	return;
}
