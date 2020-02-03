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

extern TIM_HandleTypeDef htim1;

osThreadId BeepTaskHandle = NULL;
osMessageQId BeepTaskQHandle;

/* *************************************************************************
 * void StartBeepTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartBeepTask(void* argument)
{
//	BaseType_t Qret;	// queue receive return
	struct BEEPQ beep;
	int i;

//osDelay(1000);

  /* Infinite loop */
  for(;;)
  {
		/* Wait indefinitely for someone to load something into the queue */
		/* Skip over empty returns, and zero time durations */
		xQueueReceive(BeepTaskQHandle,&beep,portMAX_DELAY);

		for (i = 0; i < beep.repct; i++)
		{
			if (beep.duron > 0)
			{
				/* Turn beeper ON. */
				HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);  
				osDelay(beep.duron); // ON duration of beeper
			}
			if (beep.duroff > 0)
			{
				/* Turn Beeper off */
				HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);  
				osDelay(beep.duroff); // ON duration of beeper
			}
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
     64, NULL, taskpriority, &BeepTaskHandle);
	if (ret != pdPASS) return NULL;

	BeepTaskQHandle = xQueueCreate(beepqsize, sizeof(struct BEEPQ) );
	if (BeepTaskQHandle == NULL) return NULL;

	return BeepTaskHandle;
}


