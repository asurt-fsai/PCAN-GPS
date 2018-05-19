#ifndef _UART_H
#define _UART_H

#include <system_LPC407x_8x_177x_8x.h>
#include <lpc407x_8x_177x_8x.h>

#define UART0_BR 		115200	/* UART0 bit rate */
#define UART0_TXB		128		/* Size of Tx buffer */
#define UART0_RXB		128		/* Size of Rx buffer */
#define LPCSC_UART0	3

#define UART1_BR 		115200
#define UART1_TXB		128
#define UART1_RXB		128
#define LPCSC_UART1	4

#define UART2_BR	 	115200
#define UART2_TXB		128
#define UART2_RXB		128
#define LPCSC_UART2	24


int uart0_test (void);
void uart0_putc (uint8_t);
uint8_t uart0_getc (void);


void Init_UART1 (void);
int uart1_test (void);
void uart1_putc (uint8_t);
uint8_t uart1_getc (void);


void Init_UART2 (void);
int uart2_test (void);
void uart2_putc (uint8_t);
uint8_t uart2_getc (void);


#endif
