/******************************************************************************
* File Name          : SwitchTask.h
* Date First Issued  : 02/042020
* Description        : Updating switches from spi shift register
*******************************************************************************/

#ifndef __SWITCHTASK
#define __SWITCHTASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#define SW_ON  1
#define SW_OFF 0

struct SWPAIR
{
	uint8_t on;      // 0 = sw is off, 1 = sw is on
	 int8_t db_ctr;  // Debounce: counter
	 int8_t db_init; // Initial count
	uint8_t state;   // Switch state
};	

/* *************************************************************************/
osThreadId xSwitchTaskCreate(uint32_t taskpriority);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: SwitchTaskHandle, or NULL if queue or task creation failed
 * *************************************************************************/
void StartSwitchTask(void* argument);
/*	@brief	: Task startup
 * *************************************************************************/

extern osMessageQId SwitchTaskQHandle;
extern osThreadId   SwitchTaskHandle;

extern struct SWPAIR swpair_safeactive;

extern struct SWPAIR pb_reversetorq;
extern struct SWPAIR pb_arm;
extern struct SWPAIR pb_prep;
extern struct SWPAIR pb_zodomtr;

#endif

