/******************************************************************************
* File Name          : SpiOutTask.c
* Date First Issued  : 11/11/2019
* Description        : Common task for setting bits int spi output shift register
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "SpiOutTask.h"

#include "main.h"

osThreadId SpiOutTaskHandle = NULL;

/* Queue */
#define QUEUESIZE 32	// Queue size for requests
osMessageQId SpiOutTaskQHandle;

/* *************************************************************************
 * void StartSpiOutTask(void* argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartSpiOutTask(void* argument)
{
	struct SPIOUTREQUEST  spireq; // Copied item from queue

  /* Infinite loop */
  for(;;)
  {
		do
		{
		/* Wait indefinitely for someone to load something into the queue */
		/* Skip bogus bit numbers and on/off settings */
			xQueueReceive(SpiOutTaskQHandle,&spireq,portMAX_DELAY);
		} while ((spireq.bitnum > 15) || (spireq.on > 1));
		if (spireq.on != 0)
		{
			spisp_wr[0].u16 |= (1 << spireq.bitnum);
		}
		else
		{
			spisp_wr[0].u16 &= ~(1 << spireq.bitnum);		
		}
	}
}
/* *************************************************************************
 * osThreadId xSpiOutTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: Task Handle
 * *************************************************************************/
osThreadId xSpiOutTaskCreate(uint32_t taskpriority)
{
	BaseType_t ret = xTaskCreate(StartSpiOutTask, "SpiOutTask",\
     (64), NULL, taskpriority,\
     &SpiOutTaskHandle);
	if (ret != pdPASS) return NULL;

	SpiOutTaskQHandle = xQueueCreate(QUEUESIZE, sizeof(struct SPIOUTREQUEST) );
	if (SpiOutTaskQHandle == NULL) return NULL;
	return SpiOutTaskHandle;
}

