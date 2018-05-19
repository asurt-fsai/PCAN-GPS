/*------------------------------------------------------------------------/
/  LPC178x UART control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2013, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------*/

#include "uart407x.h"


#define F_PCLK		120000000	/* PCLK frequency for the UART module */
#define	DIVADD		5			/* See below */
#define	MULVAL		13

#define	DLVAL0		((uint32_t)((double)F_PCLK / UART0_BR / 16 / (1 + (double)DIVADD / MULVAL)))
#define	DLVAL1		((uint32_t)((double)F_PCLK / UART1_BR / 16 / (1 + (double)DIVADD / MULVAL)))
#define	DLVAL2		((uint32_t)((double)F_PCLK / UART2_BR / 16 / (1 + (double)DIVADD / MULVAL)))


static volatile struct {
	uint16_t	ri, wi, ct, act;
	uint8_t		buff[UART0_TXB];
} TxBuff0;

static volatile struct {
	uint16_t	ri, wi, ct;
	uint8_t		buff[UART0_RXB];
} RxBuff0;




int uart0_test (void)
{
	return RxBuff0.ct;
}


uint8_t uart0_getc (void)
{
	uint8_t d;
	int i;

	/* Wait while Rx buffer is empty */
	while (!RxBuff0.ct) ;

	i = RxBuff0.ri;			/* Get a byte from Rx buffer */
	d = RxBuff0.buff[i++];
	RxBuff0.ri = i % UART0_RXB;
	__disable_irq();
	RxBuff0.ct--;
	__enable_irq();

	return d;
}


void uart0_putc (uint8_t d)
{
	int i;

	/* Wait for Tx buffer ready */
	while (TxBuff0.ct >= UART0_TXB) ;

	__disable_irq();
	if (TxBuff0.act) {
		i = TxBuff0.wi;		/* Put a byte into Tx byffer */
		TxBuff0.buff[i++] = d;
		TxBuff0.wi = i % UART0_TXB;
		TxBuff0.ct++;
	} else {
		LPC_UART0->THR = d;		/* Trigger Tx sequense */
		TxBuff0.act = 1;
	}
	__enable_irq();
}







static volatile struct {
	uint16_t	ri, wi, ct, act;
	uint8_t		buff[UART1_TXB];
} TxBuff1;

static volatile struct {
	uint16_t	ri, wi, ct;
	uint8_t		buff[UART1_RXB];
} RxBuff1;


int uart1_test (void)
{
	return RxBuff1.ct;
}


uint8_t uart1_getc (void)
{
	uint8_t d;
	int i;

	/* Wait while Rx buffer is empty */
	while (!RxBuff1.ct) ;

	i = RxBuff1.ri;			/* Get a byte from Rx buffer */
	d = RxBuff1.buff[i++];
	RxBuff1.ri = i % UART1_RXB;
	__disable_irq();
	RxBuff1.ct--;
	__enable_irq();

	return d;
}


void uart1_putc (uint8_t d)
{
	int i;

	/* Wait for Tx buffer ready */
	while (TxBuff1.ct >= UART1_TXB) ;

	__disable_irq();
	if (TxBuff1.act) {
		i = TxBuff1.wi;		/* Put a byte into Tx byffer */
		TxBuff1.buff[i++] = d;
		TxBuff1.wi = i % UART1_TXB;
		TxBuff1.ct++;
	} else {
		LPC_UART1->THR = d;		/* Trigger Tx sequense */
		TxBuff1.act = 1;
	}
	__enable_irq();
}


static volatile struct {
	uint16_t	ri, wi, ct, act;
	uint8_t		buff[UART2_TXB];
} TxBuff2;

static volatile struct {
	uint16_t	ri, wi, ct;
	uint8_t		buff[UART2_RXB];
} RxBuff2;


int uart2_test (void)
{
	return RxBuff2.ct;
}


uint8_t uart2_getc (void)
{
	uint8_t d;
	int i;

	/* Wait while Rx buffer is empty */
	while (!RxBuff2.ct) ;

	i = RxBuff2.ri;			/* Get a byte from Rx buffer */
	d = RxBuff2.buff[i++];
	RxBuff2.ri = i % UART2_RXB;
	__disable_irq();
	RxBuff2.ct--;
	__enable_irq();

	return d;
}


void uart2_putc (uint8_t d)
{
	int i;

	/* Wait for Tx buffer ready */
	while (TxBuff2.ct >= UART2_TXB) ;

	__disable_irq();
	if (TxBuff2.act) {
		i = TxBuff2.wi;		/* Put a byte into Tx byffer */
		TxBuff2.buff[i++] = d;
		TxBuff2.wi = i % UART2_TXB;
		TxBuff2.ct++;
	} else {
		LPC_UART2->THR = d;		/* Trigger Tx sequense */
		TxBuff2.act = 1;
	}
	__enable_irq();
}




