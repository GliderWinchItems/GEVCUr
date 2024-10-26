/******************************************************************************
* File Name          : LcdmsgsetTask.h
* Date First Issued  : 04/22/2020
* Description        : lcdi2c printf calling
*******************************************************************************/

#ifndef __LCDMSGSETTASK
#define __LCDMSGSETTASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* 
This union provides a way to pass some variables to a function that uses vsnprintf.
*/
union LCDSETVAR
{
	float f;
	uint32_t u32;
	 int32_t s32;
	uint16_t u16[2];
	 int16_t s16[2];
	uint8_t   u8[4];
	 int8_t   s8[4];
//These added to allow two floats to be passed	 
	float ftwo[2];    
	uint32_t u32two[2];
	uint64_t u64;
};

/*
This struct is placed on a queue and the pointer is set to point to a function
that calls vsnprintf in LcdmsgssetTask.c. Any waits on the semaphore for
vsnprint occur in LcdmsgsetTask.c and not the originating task that wants to
output something.
*/
struct LCDMSGSET
{
	void (*ptr)(union LCDSETVAR);
	union LCDSETVAR u;
};

 /* *************************************************************************/
 osThreadId xLcdmsgsetTaskCreate(uint32_t taskpriority, uint16_t numbcb);
 /* @brief	: Create task; task handle created is global for all to enjoy!
  * @param	: taskpriority = Task priority (just as it says!)
  * @param	: numbcb = number of message requests allowed in queue
  * @return	: LcdmsgsTaskHandle
  * *************************************************************************/

extern osMessageQId LcdmsgsetTaskQHandle;
extern TaskHandle_t LcdmsgsetTaskHandle;

#endif
