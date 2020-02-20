/******************************************************************************
* File Name          : SpiOutTask.h
* Date First Issued  : 11/11/2019
* Description        : Common task for setting bits int spi output shift register
*******************************************************************************/

#ifndef __SPIOUTTASK
#define __SPIOUTTASK

#include "FreeRTOS.h"
#include "task.h"
#include "spiserialparallelSW.h"

struct SPIOUTREQUEST
{
	uint8_t bitnum;	// Bit number (0 - 15)
	uint8_t on;       // Set on = 1, set off = 0
};

/* *************************************************************************/
void StartSpiOutTask(void* argument);
/*	@brief	: Task startup
 * *************************************************************************/
osThreadId xSpiOutTaskCreate(uint32_t taskpriority);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: Task Handle
 * *************************************************************************/

extern osMessageQId SpiOutTaskQHandle;
extern osThreadId SpiOutTaskHandle;

#endif

