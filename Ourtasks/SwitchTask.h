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

struct SWPAIR
{
	uint8_t no;      // Normally open:   0 = not active, 1 = active
	uint8_t nc;      // Normally closed: 0 = not active, 1 = active
	uint8_t db_init; // Debounce: initial count
	uint8_t db_ctr;  // Debounce: counter
	uint8_t state;
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

#endif

