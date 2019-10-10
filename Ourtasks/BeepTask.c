/******************************************************************************
* File Name          : BeepTask.c
* Date First Issued  : 10/01/2019
* Description        : MC hw beeper
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"

#include "BeepTask.h"

#include "stm32f4xx_hal_usart.h"
#include "stm32f4xx_hal_uart.h"

osThreadId BeepTaskHandle = NULL;
osMessageQId BeepTaskSendQHandle;

/* *************************************************************************
 * void StartBeepTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartBeepTask(void* argument)
{
	BaseType_t Qret;	// queue receive return
	struct BEEPQ* pbeep;
	int i;

  /* Infinite loop */
  for(;;)
  {
		do
		{
		/* Wait indefinitely for someone to load something into the queue */
		/* Skip over empty returns, and zero time durations */
			Qret = xQueueReceive(BeepTaskQHandle,&pbeep,portMAX_DELAY);
			if (Qret == pdPASS) // Break loop if not empty
				break;
		} while ((pbeep->duron == 0) || (pbeep->duroff == 0));

		for (i = 0; i < pbeep->repct; i++)
		{
			/* Turn beeper ON. */
			HAL_GPIO_WritePin(BEEPPORT, BEEPPIN, GPIO_PIN_SET);
			osDelay(pbeep->duron); // ON duration of beeper

			/* Turn Beeper off */
			HAL_GPIO_WritePin(BEEPPORT, BEEPPIN, GPIO_PIN_RESET);
			osDelay(pbeep->duroff); // ON duration of beeper
		}
	}
}
/* *************************************************************************
 * osThreadId xBeepTaskCreate(uint32_t taskpriority, uint32_t beepqsize);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: BeepTaskHandle
 * *************************************************************************/
osThreadId xBeepTaskCreate(uint32_t taskpriority, uint32_t beepqsize)
{
	BaseType_t ret = xTaskCreate(&StartBeepTask, "BeepTask",\
     192, NULL, taskpriority, &BeepTaskHandle);
	if (ret != pdPASS) return NULL;

	BeepTaskSendQHandle = xQueueCreate(beepqsize, sizeof(struct BEEPQ) );
	if (BeepTaskSendQHandle == NULL) return NULL;

	return BeepTaskHandle;
}


