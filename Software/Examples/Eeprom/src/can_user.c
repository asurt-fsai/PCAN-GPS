
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
#include "can.h"
#include "can_user.h"


/*******************************************************************************
 global definitions
 ******************************************************************************/


/*******************************************************************************
 local definitions
 ******************************************************************************/


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
