
#ifndef TYPEDEFS_H
#define TYPEDEFS_H
/***************************************************************************//**
 * @file	typedefs.h
 * @brief	Project related type definitions
 * @version	1.0.0
 * @date		18. July 2014
 * @author	PEAK-SYSTEM TECHNIK
 *
 * Copyright (c): PEAK-SYSTEM TECHNIK GMBH, DARMSTADT
 *****************************************************************************
 *
 *	History: (newer entries first!)
 *
 *	dd-mm-yy/Sgn.			Vers.	changes made
 *
 *	13-Nov-2013/StM		0.0		start of development
 *
 ****************************************************************************/

/*****************************************************************************
                                include files
 ****************************************************************************/
 
/*****************************************************************************
                        type definitions export section
 ****************************************************************************/
//----------------------------------------------------------------------------
// Global type definitions in the PCAN-GPS project
//----------------------------------------------------------------------------
#define sizeofmember(type, member) sizeof(((type *)0)->member)

typedef unsigned char u8_t;
typedef	char b8_t;			// blank
typedef unsigned short u16_t;
typedef unsigned long u32_t;
typedef unsigned long long u64_t;
typedef signed char s8_t;
typedef signed short s16_t;
typedef signed long s32_t;

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif
#define	_BV(bit) (1<<(bit))

// unions to map the four data types
typedef union {
	u8_t  b[8];
	u16_t w[4];
	u32_t l[2];
	u64_t ll;
} U_LONGLONG;

typedef union {
	u8_t b[4];
	u16_t w[2];
	u32_t l;
} U_LONGWORD;


//<! common return values
#define RET_ERR -1
#define RET_OK 	0

#endif /* TYPEDEFS_H_ */
