/******************************************************************************
* File Name          : BeepTask.h
* Date First Issued  : 10/01/2019
* Description        : MC hw beeper
*******************************************************************************/

#ifndef __BEEPTASK
#define __BEEPTASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#define BEEPPORT GPIOA // Port for beeper
#define BEEPPIN  8     // Port Pin for beeper

struct BEEPQ
{
	uint16_t duron;   // Duration (timer ticks) beep is ON
	uint16_t duroff;  // Duration (timer ticks) beep is OFF (before finished)
	uint16_t repct;   // Repetition ct of this beep
};

/* *************************************************************************/
osThreadId xBeepTaskCreate(uint32_t taskpriority, uint32_t beepqsize);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: BeepTaskHandle
 * *************************************************************************/
void StartBeepTask(void* argument);
/*	@brief	: Task startup
 * *************************************************************************/


extern osMessageQId BeepTaskQHandle;
extern osThreadId   BeepTaskHandle;

#endif

