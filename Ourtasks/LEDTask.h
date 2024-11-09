/******************************************************************************
* File Name          : LEDTask.h
* Date First Issued  : 01/31/2020
* Description        : SPI/LED control 
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
#define LED_BLINKWINK 5

struct LEDREQ
{
	uint8_t bitnum;	// Bit number (0 - 15)
	uint8_t mode;     // LED mode code
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

