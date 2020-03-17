/******************************************************************************
* File Name          : lcdprintf.h
* Date First Issued  : 10/01/2019
* Description        : LCD display printf
*******************************************************************************/

#ifndef __LCDPRINTF
#define __LCDPRINTF

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

struct 

/* *************************************************************************/
osThreadId xLcdTaskCreate(uint32_t taskpriority);
/* osThreadId xLcdTaskCreate(uint32_t taskpriority, uint32_t beepqsize);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: LcdTaskHandle
 * *************************************************************************/
 void vStartLcdTask(void);
/*	@brief	: Task startup
 * *************************************************************************/
osThreadId xLcdTaskCreate(uint32_t taskpriority, uint32_t beepqsize);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: LcdTaskHandle
 * *************************************************************************/

extern osMessageQId LcdTaskQHandle;
extern osThreadId   LcdTaskHandle;

/* Macros for simplifying the wait and loading of the queue */
#define mLcdTaskWait( noteval, bit){while((noteval & bit) == 0){xTaskNotifyWait(bit, 0, &noteval, 5000);}}
#define mLcdTaskQueueBuf(bcb){uint32_t qret;do{qret=xQueueSendToBack(LcdTaskQHandle,bcb,portMAX_DELAY);}while(qret == errQUEUE_FULL);}

#define mLcdTaskQueueBuf2(bcb,bit){uint32_t qret;do{noteval&=~bit;qret=xQueueSendToBack(LcdTaskQHandle,bcb,portMAX_DELAY);}while(qret == errQUEUE_FULL);}

#endif

