/******************************************************************************
* File Name          : lcdmsgset.c
* Date First Issued  : 04/21/2020
* Description        : Conslidate LCD printf'ing
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"

osMessageQId lcdmsgsetQHandle;

/* *************************************************************************
 * osMessageQId lcdmsgset_init(uint16_t qsize);
 *	@brief	: Setup the queue for pointers
 * @return	: NULL = failed; pointer to queue handle
 * *************************************************************************/
 osMessageQId lcdmsgset_init(uint16_t qsize)
{
	lcdmsgQHandle = xQueueCreate(qsize, sizeof(struct LCDMSGSET) );
	if (lcdmsgsetQHandle == NULL) return NULL;
	return lcdmsgsetQHandle;
}

/* *************************************************************************
 * void lcdmsgset_poll(void);
 *	@brief	: 
 * *************************************************************************/
void lcdmsgset_poll(void)
{
	union LCDSETVAR lsv;

	lms.ptr = NULL;

	BaseType_t ret;

	ret = xQueueReceive(lcdmsgsetQHandle,&lsv,0);
	if (ret == errQUEUE_EMPTY) return;

		if (ptr != NULL) // jic a NULL ptr got on the queue
		  (*ptr)(lv.u);	// Go do something

	return;
}
 /* *************************************************************************
 * osThreadId xLcdmsgsTaskCreate(uint32_t taskpriority, uint16_t numbcb);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @param	: numbcb = number of message requests allowed in queue
 * @return	: LcdmsgsTaskHandle
 * *************************************************************************/
osThreadId xLcdmsgsetTaskCreate(uint32_t taskpriority, uint16_t numbcb)
{
	BaseType_t ret = xTaskCreate(&StartLcdmsgsTask,"LcdI2CTask",512,NULL,taskpriority,&LcdmsgsTaskHandle);
	if (ret != pdPASS) morse_trap(35);//return NULL;

	LcdmsgsTaskQHandle = xQueueCreate(numbcb, sizeof(struct LCDMSGTASK_MSGREQ) );
	if (LcdmsgsTaskQHandle == NULL) morse_trap(37); //return NULL;


	return LcdmsgsTaskHandle;
}
