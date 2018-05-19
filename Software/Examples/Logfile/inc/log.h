/**
 * @file log.h
 * @brief Example_source
 */


/***************************************************************************
 *
 * Project  : PCAN-GPRS Link
 * Module   : Log
 * Filename : log.h
 * System   : LPC2368 (ARM7)
 * Compiler : Realview Armcc
 * Switches : see project file
 * Author   :
 * Rights   : PEAK-System Technik GmbH
 *                www.peak-system.com
 *
 ****************************************************************************
 * Implementation description
 *
 *   Datatypes definitions.
 *
 ****************************************************************************
 * History: (newer entries first!)
 *----------------------------------------------------------------------------
 * Date / Name         Vers.   changes made
 *----------------------------------------------------------------------------
 *
 ***************************************************************************/

#ifndef LOG_H_
#define LOG_H_



/*****************************************************************************
                                 include files
 ****************************************************************************/

/*****************************************************************************
                             global data definitions
 ****************************************************************************/
 
//--------------------------------------------------------------------------
// State machine
//--------------------------------------------------------------------------
#define LOG_INIT 	1
#define LOG_TASK 	2
#define LOG_START 	3
#define LOG_STOP 	4
#define LOG_NO_LOG 	5
#define LOG_ERROR 	6
#define LOG_NOCARD 7

#define LOG_FLG_DISK_INIT_OK	1<<0
#define LOG_FLG_FS_INIT_OK		1<<1
#define LOG_FLG_CWD_OK			1<<2
#define LOG_FLG_FILE_OPEN		1<<3

#define LOG_DIR_NAME "log_data"

/*****************************************************************************
                           global function prototypes
 ****************************************************************************/

//----------------------------------------------------------------------------
//	void Log_task(void)
//----------------------------------------------------------------------------
//! @brief			creates Log-File and logs the data current GPS-Data
//! @attention		initialized Memory-Card required
//---------------------------------------------------------------------------- 
void Log_task(void);


#endif /* LOG_H_ */
