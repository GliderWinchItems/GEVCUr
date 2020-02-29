/******************************************************************************
* File Name          : lcdmsg.c
* Date First Issued  : 02/29/2020
* Description        : Conslidate LCD printf'ing
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"

osMessageQId lcdmsgQHandle;

/* *************************************************************************
 * osMessageQId lcdmsg_init(uint16_t qsize);
 *	@brief	: Setup the queue for pointers
 * @return	: NULL = failed; pointer to queue handle
 * *************************************************************************/
 osMessageQId lcdmsg_init(uint16_t qsize)
{
	lcdmsgQHandle = xQueueCreate(qsize, sizeof(void*) );
	if (lcdmsgQHandle == NULL) return NULL;
	return lcdmsgQHandle;
}

/* *************************************************************************
 * void lcdmsg_poll(void);
 *	@brief	: 
 * *************************************************************************/
void lcdmsg_poll(void)
{
	void (*ptr)(void) = NULL;

	BaseType_t ret;

	ret = xQueueReceive(lcdmsgQHandle,&ptr,0);
	if (ret == errQUEUE_EMPTY) return;

		if (ptr != NULL) // jic a NULL ptr got on the queue
		  (*ptr)();	// Go do something

	return;
}
