/**
 * @file log.c
 * @brief
 */

/***************************************************************************
 *
 * Project	: PCAN-GPS
 * Module	: log
 * Filename	: log.c
 * System	: LPC4074
 * Compiler	: GCC
 * Switches	: see project file
 * Author	:
 * Rights	: PEAK-System Technik GmbH
 *            www.peak-system.com
 *
 ****************************************************************************
 * Implementation description
 *
 *
 *
 ****************************************************************************
 * History: (newer entries first!)
 *----------------------------------------------------------------------------
 * Date / Name         Vers.   	changes made
 * 18.07.2014 Michaelsen			0.1		first implementation
 *----------------------------------------------------------------------------

 ***************************************************************************/

/*****************************************************************************
                                include files
 ****************************************************************************/
//
// System header files
//
#include "typedefs.h"
#include <system_LPC407x_8x_177x_8x.h>
#include <lpc407x_8x_177x_8x.h>

//
// Library header files
//


//
// Source code header files
//
#include "hardware.h"
#include "timer.h"
#include "ff.h"
#include "diskio.h"
#include "log.h"
#include "xprintf.h"
#include "MAX-7W.h"
/*****************************************************************************
                             global definitions
 ****************************************************************************/
//--------------------------------------------------------------------------
// FAT file system structure
//--------------------------------------------------------------------------
// File system object for each logical drive
FATFS fatfs[_VOLUMES];
// File objects
FIL logFile={0};
DIR dir;
FILINFO Finfo;
u16_t AccSize;			
u32_t AccFiles, AccDirs;


u32_t fs_state; 

static u32_t logState = LOG_INIT;

//--------------------------------------------------------------------------
static u8_t nameBuff[32] = { "traceXXXX.bin"};

static u32_t fileSize;

#define LOG_BUFFER_SZ 1024
static char log_buffer[LOG_BUFFER_SZ]; 	//!< buffer to collect data and write lager amounts at once
static unsigned int cur_buff_pos=0;		//!< current position in log_buffer

/*****************************************************************************
                          	local definitions
 ****************************************************************************/
#define CREATE_FILE

/*****************************************************************************
                          local function prototypes
 ****************************************************************************/
void write_data(u32_t btw, void* buff, FIL* p_File, u32_t* FileSize);
void log_data(void);

/*****************************************************************************
                               global functions
 ****************************************************************************/
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------
void Log_task(void)
{
	FRESULT res;         // FatFs function common result code

	switch(logState)
	{
		case LOG_INIT:
			// Initializing disk
			if(!(fs_state & LOG_FLG_DISK_INIT_OK)){
				res = disk_initialize((BYTE)0);
				if(res==FR_OK)
					fs_state|=LOG_FLG_DISK_INIT_OK;
				return;
			}
		
			// Mount file system:
			if(!(fs_state & LOG_FLG_FS_INIT_OK)){
				res = f_mount(&fatfs[0], "", (BYTE)1);
				if(res==FR_OK)
					fs_state|=LOG_FLG_FS_INIT_OK;
				return;
			}
			
			// Check GPS date and time
			if(!(MAX7W_Readings.Validity & GPS_DATE_VALID))
				return;
			if(!(MAX7W_Readings.Validity & GPS_TIME_VALID))
				return;
			
			// Change working directory
			if(!(fs_state & LOG_FLG_CWD_OK)){
				res = (f_chdir(LOG_DIR_NAME));
				if (res==FR_NO_PATH){ 
					// Try to create directory
					res = f_mkdir(LOG_DIR_NAME);
					return;
				}else{
					fs_state|=LOG_FLG_CWD_OK;
				}
				return;
			}
			
			// Open trace file
			if(!(fs_state & LOG_FLG_FILE_OPEN)){
#if _USE_LFN
				xsprintf((char*)nameBuff,"trc%04u%02u%02u_%02u%02u%02u.trx",
															MAX7W_Readings.Date_Year+2000,
															MAX7W_Readings.Date_Month,
															MAX7W_Readings.Date_DayOfMonth,
															MAX7W_Readings.Time_Hrs+2,
															MAX7W_Readings.Time_Min,
															MAX7W_Readings.Time_Sec);
#else
				//only 8.3 names are allowed
				xsprintf((char*)nameBuff,"%02u%02u%02u%02u.trx",
															MAX7W_Readings.Date_Month, 
															MAX7W_Readings.Date_DayOfMonth,
															MAX7W_Readings.Time_Hrs,
															MAX7W_Readings.Time_Min);
#endif
				res = f_open(&logFile,(char*)nameBuff,(FA_CREATE_NEW | FA_WRITE));
				if(res == FR_OK){
					fs_state |= LOG_FLG_FILE_OPEN;
				}else{
				}
				return;
			}
			
			// Update the file system
			f_sync(&logFile);

			// Go to next state
			logState = LOG_START;
			break;
			
		case LOG_START:
			// write file header
			write_header();
			logState=LOG_TASK;
			break;

		case LOG_TASK:
			log_data();
			break;

		case LOG_STOP:
			// If file footer is needed it could be written here
			logState=LOG_NO_LOG;
			break;

		case LOG_NO_LOG:
			break;

		case LOG_ERROR:
			
			break;

		default:

			break;
	}//LOG STATE
	return;
}


/*****************************************************************************
                                local functions
 ****************************************************************************/

//--------------------------------------------------------------------------
//! @brief 
//--------------------------------------------------------------------------
void log_data(void){
	u32_t btw;
	u32_t lon_deg, lon_dec, lon_frac;
	u32_t lat_deg, lat_dec, lat_frac;
	u32_t alt_dec, alt_frac;
	u32_t pdop_dec, pdop_frac;
	u32_t hdop_dec, hdop_frac;
	u32_t vdop_dec, vdop_frac;
	char local_buffer[128];
	
	// only log data if GPS has a fix
	if(MAX7W_Readings.Nav_Method != 2 && MAX7W_Readings.Nav_Method != 3 )
		return;
	lon_deg = (u32_t)MAX7W_Readings.Pos_Longitude/100;
	lon_dec = (u32_t)(MAX7W_Readings.Pos_Longitude-(float)(lon_deg*100));
	lon_frac = (u32_t)((MAX7W_Readings.Pos_Longitude-(float)lon_dec-(float)(lon_deg*100))*100000);
	lat_deg = (u32_t)MAX7W_Readings.Pos_Latitude/100;
	lat_dec = (u32_t)(MAX7W_Readings.Pos_Latitude-(float)(lat_deg*100));
	lat_frac = (u32_t)((MAX7W_Readings.Pos_Latitude-(float)lat_dec-(float)(lat_deg*100))*100000);
	alt_dec = (u32_t)MAX7W_Readings.Pos_AltitudeOverSea;
	alt_frac = (u32_t)((MAX7W_Readings.Pos_AltitudeOverSea-(float)alt_dec)*1000);

	pdop_dec = (u32_t)MAX7W_Readings.PDOP;
	pdop_frac = (u32_t)((MAX7W_Readings.PDOP-(float)pdop_dec)*1000);
	hdop_dec = (u32_t)MAX7W_Readings.HDOP;
	hdop_frac = (u32_t)((MAX7W_Readings.HDOP-(float)hdop_dec)*1000);
	vdop_dec = (u32_t)MAX7W_Readings.VDOP;
	vdop_frac = (u32_t)((MAX7W_Readings.VDOP-(float)vdop_dec)*1000);

	xsprintf(local_buffer, "%u;%c %u %02u.%05u;%c %u %02u.%05u;%u.%03u;%u:%u:%u;%u.%03u;%u.%03u;%u.%03u\n",
			SYSTIME_NOW,
			MAX7W_Readings.Pos_LongitudeIndEW, lon_deg, lon_dec, lon_frac,
			MAX7W_Readings.Pos_LatitudeIndNS, lat_deg, lat_dec, lat_frac,
			alt_dec, alt_frac,
			MAX7W_Readings.Time_Hrs, MAX7W_Readings.Time_Min, MAX7W_Readings.Time_Sec,
			pdop_dec, pdop_frac,
			hdop_dec, hdop_frac,
			vdop_dec, vdop_frac );
	
	btw = strlen(local_buffer);

	if(btw>=(LOG_BUFFER_SZ-cur_buff_pos)){
		// not enough space in log buffer so write data to file first
		HW_SetLED (HW_LED_STATUS_2, HW_LED_GREEN);
		write_data(cur_buff_pos,(void*) log_buffer, &logFile, &fileSize);
		cur_buff_pos = 0;
		HW_SetLED (HW_LED_STATUS_2, HW_LED_OFF);
	}

	// copy data to log buffer
	memcpy(&log_buffer[cur_buff_pos], local_buffer,btw);
	cur_buff_pos+=btw;

	return;
}



//--------------------------------------------------------------------------
//! @brief
//--------------------------------------------------------------------------
void write_header(FIL* p_File, u32_t* FileSize){
	FRESULT res;         // FatFs function common result code
	u32_t  bw=0;

	#define HEADER  "system time [us];"	\
					"Longitude;"		\
					"Latitude;"			\
					"Altitude;"			\
					"Time;"				\
					"PDOP;"				\
					"HDOP;"				\
					"VDOP\n"

	memcpy(&log_buffer[0], HEADER,strlen(HEADER));
	cur_buff_pos=strlen(HEADER);

	return;
}



//--------------------------------------------------------------------------
//! @brief
//--------------------------------------------------------------------------
void write_data(u32_t btw,void* buff, FIL* p_File, u32_t* FileSize){
	FRESULT res;         // FatFs function common result code
	u32_t  bw=0;

	//Place write Pointer
	res = f_lseek(p_File,*FileSize);
	//Update file size
	*FileSize += btw;

	//Write buffer to the SD-Card
	res = f_write(p_File,buff,btw,(UINT *)&bw);
	//Check if the write was ok
	if((bw < btw )|| res)
	{
		logState = LOG_ERROR;
	}
	//Update the FAT
	f_sync(p_File);
}




