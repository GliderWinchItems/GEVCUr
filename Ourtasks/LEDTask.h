/******************************************************************************
* File Name          : LEDTask.h
* Date First Issued  : 01/31/2020
* Description        : GSM Control Panel LED 
*******************************************************************************/

#ifndef __LEDTASK
#define __LEDTASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

/* LED modes. */
#define LED_OFF       0
#define LED_ON        1
#define LED_BLINKSLOW 2
#define LED_BLINKFAST 3
#define LED_BLINK1SEC 4

struct LEDREQ
{
	uint8_t bitnum;	// Bit number (0 - 15)
	uint8_t mode;     // 
		// 0 = off
		// 1 = on 
		// 3 = blink slow
		// 4 = blink fast
		// 5 = 1 sec on/off	

};


/* *************************************************************************/
osThreadId xLEDTaskCreate(uint32_t taskpriority, uint32_t ledqsize);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: LEDTaskHandle
 * *************************************************************************/
void StartLEDTask(void* argument);
/*	@brief	: Task startup
 * *************************************************************************/


extern osMessageQId LEDTaskQHandle;
extern osThreadId   LEDTaskHandle;

#endif

