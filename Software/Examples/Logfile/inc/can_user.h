/***************************************************************************//**
* @file		can_user.h
* @brief	User level CAN functions
* @version	1.0.0
* @date		18. July 2014
* @author	PEAK-SYSTEM TECHNIK
*
* Copyright (c): PEAK-SYSTEM TECHNIK GMBH, DARMSTADT
* *****************************************************************************/


#ifndef  _CAN_USER_H_
#define  _CAN_USER_H_

// defines
#define  CAN_TX_QUEUE_SIZE	16
#define  CAN_RX_QUEUE_SIZE	8

// Baudrates
// VPB clock 60 MHz, 15 Tsegs, sample point 80 %
//								-- SJW --	- Tseg1 -	- Tseg2 -	- BRP -
#define		CAN_BAUD_1M			(	0 << 14 |	10 << 16 |	2 << 20 |	7)
#define		CAN_BAUD_800K		(	0 << 14 |	10 << 16 |	2 << 20 |	9)
#define		CAN_BAUD_500K		(	0 << 14 |	10 << 16 |	2 << 20 |	15)
#define		CAN_BAUD_250K		(	0 << 14 |	10 << 16 |	2 << 20 |	31)
#define		CAN_BAUD_200K		(	0 << 14 |	10 << 16 |	2 << 20 |	39)
#define		CAN_BAUD_125K		(	0 << 14 |	10 << 16 |	2 << 20 |	63)
#define		CAN_BAUD_100K		(	0 << 14 |	10 << 16 |	2 << 20 |	79)
#define		CAN_BAUD_95K2		(	0 << 14 |	10 << 16 |	2 << 20 |	83)
#define		CAN_BAUD_83K3		(	0 << 14 |	10 << 16 |	2 << 20 |	95)
#define		CAN_BAUD_50K		(	0 << 14 |	10 << 16 |	2 << 20 |	159)
#define		CAN_BAUD_47K6		(	0 << 14 |	10 << 16 |	2 << 20 |	167)
#define		CAN_BAUD_33K3		(	0 << 14 |	10 << 16 |	2 << 20 |	239)
#define		CAN_BAUD_20K		(	0 << 14 |	10 << 16 |	2 << 20 |	399)
#define		CAN_BAUD_10K		(	0 << 14 |	10 << 16 |	2 << 20 |	799)

// user function prototypes

CANStatus_t CAN_UserWrite(CANMsg_t *pBuff);

u32_t CAN_UserRead(CANMsg_t *pBuff);

void CAN_UserInit(void);

void CAN_UserSendGPSData(void);


#endif
