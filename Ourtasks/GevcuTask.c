/******************************************************************************
* File Name          : GevcuTask.c
* Date First Issued  : 10/08/2019
* Description        : Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"
#include "morse.h"
#include "SerialTaskReceive.h"
#include "GevcuTask.h"
#include "gevcu_idx_v_struct.h"
#include "GevcuEvents.h"
#include "GevcuStates.h"
#include "GevcuUpdates.h"
#include "gevcu_func_init.h"

/* From 'main.c' */
extern UART_HandleTypeDef huart3;

#define SWTIM1 500

osThreadId GevcuTaskHandle;

struct GEVCUFUNCTION gevcufunction;

/* *************************************************************************
 * void swtim1_callback(TimerHandle_t tm);
 * @brief	: Software timer 1 timeout callback
 * *************************************************************************/
static void swtim1_callback(TimerHandle_t tm)
{
	xTaskNotify(GevcuTaskHandle, GEVCUBIT04, eSetBits);
	return;
}
/* *************************************************************************
 * void swtim2_callback(TimerHandle_t tm);
 * @brief	: Software timer 2 timeout callback
 * *************************************************************************/
static void swtim2_callback(TimerHandle_t tm)
{
	xTaskNotify(GevcuTaskHandle, GEVCUBIT05, eSetBits);
	return;
}
/* *************************************************************************
 * void swtim3_callback(TimerHandle_t tm);
 * @brief	: Software timer 3 timeout callback
 * *************************************************************************/
static void swtim3_callback(TimerHandle_t tm)
{
	xTaskNotify(GevcuTaskHandle, GEVCUBIT03, eSetBits);
	return;
}
/* *************************************************************************
 * osThreadId xGevcuTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: GevcuTaskHandle
 * *************************************************************************/
osThreadId xGevcuTaskCreate(uint32_t taskpriority)
{
 	osThreadDef(GevcuTask, StartGevcuTask, osPriorityNormal, 0, 256);
	GevcuTaskHandle = osThreadCreate(osThread(GevcuTask), NULL);
	vTaskPrioritySet( GevcuTaskHandle, taskpriority );
	return GevcuTaskHandle;
}
/* *************************************************************************
 * void StartGevcuTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartGevcuTask(void const * argument)
{
	/* Convenience pointer. */
	struct GEVCUFUNCTION* pcf = &gevcufunction;

	/* A notification copies the internal notification word to this. */
	uint32_t noteval = 0;    // Receives notification word upon an API notify
	uint32_t noteuse = 0xffffffff;

	/* Setup serial receive for uart */
	/* Get buffer control block for incoming uart lines. */
	// 2 line buffers of 48 chars, no dma buff, char-by-char line mode
//	pcf->prbcb3  = xSerialTaskRxAdduart(&huart3,0,GEVCUBIT01,&noteval,2,48,0,0);
//	if (pcf->prbcb3 == NULL) morse_trap(47);

	/* Init struct with working params */
	gevcu_idx_v_struct_hardcode_params(&gevcufunction.lc);

	/* Initialize working struc for GevcuTask. */
	extern struct ADCFUNCTION adc1;
	gevcu_func_init_init(pcf, &adc1);

	/* CAN hardware filter: restrict incoming to necessary CAN msgs. */
//$	gevcu_func_init_canfilter(pcf);
      
	/* Create timer for keep-alive.  Auto-reload/periodic */
	pcf->swtimer1 = xTimerCreate("swtim1",pcf->ka_k,pdTRUE,\
		(void *) 0, swtim1_callback);
	if (pcf->swtimer1 == NULL) {morse_trap(41);}

	/* Create timer for other delays. One-shot */
	pcf->swtimer2 = xTimerCreate("swtim2",10,pdFALSE,\
		(void *) 0, &swtim2_callback);
	if (pcf->swtimer2 == NULL) {morse_trap(42);}

	/* Create timer uart RX keep-alive. One-shot */
	pcf->swtimer3 = xTimerCreate("swtim3",30,pdFALSE,\
		(void *) 0, &swtim3_callback);
	if (pcf->swtimer3 == NULL) {morse_trap(43);}

	/* Start command/keep-alive timer */
	BaseType_t bret = xTimerReset(pcf->swtimer1, 10);
	if (bret != pdPASS) {morse_trap(44);}

	/* Upon startup state */
	pcf->state = 0;

if (pcf->evstat != 0) morse_trap(46); // Debugging check

  /* Infinite loop */
  for(;;)
  {
		/* Wait for notifications */
		xTaskNotifyWait(0,0xffffffff, &noteval, portMAX_DELAY);
//		noteused = 0;	// Accumulate bits in 'noteval' processed.
  /* ========= Events =============================== */
// NOTE: this could be made into a loop that shifts 'noteval' bits
// and calls from table of addresses.  This would have an advantage
// if the high rate bits are shifted out first since a test for
// no bits left could end the testing early.
		// Check notification and deal with it if set.
		noteuse = 0;
		if ((noteval & GEVCUBIT00) != 0)
		{ // ADC readings ready
			GevcuEvents_00(pcf);
			noteuse |= GEVCUBIT00;
		}
		if ((noteval & GEVCUBIT01) != 0)
		{ // spare
			GevcuEvents_01(pcf);
			noteuse |= GEVCUBIT01;
		}
		if ((noteval & GEVCUBIT02) != 0)
		{ // (spare)
			GevcuEvents_02(pcf);
			noteuse |= GEVCUBIT02;
		}
		if ((noteval & GEVCUBIT03) != 0)
		{ // TIMER3: (one shot)
			GevcuEvents_03(pcf);			
			noteuse |= GEVCUBIT03;
		}
		if ((noteval & GEVCUBIT04) != 0)
		{ // TIMER1:  (periodic)
			GevcuEvents_04(pcf);
			noteuse |= GEVCUBIT04;
		}
		if ((noteval & GEVCUBIT05) != 0)
		{ // TIMER2: (one shot)
			GevcuEvents_05(pcf);
			noteuse |= GEVCUBIT05;
		}
		if ((noteval & GEVCUBIT06) != 0) 
		{ // CAN:  
			GevcuEvents_06(pcf);
			noteuse |= GEVCUBIT06;
		}
		if ((noteval & GEVCUBIT07) != 0) 
		{ // CAN: 
			GevcuEvents_07(pcf);
			noteuse |= GEVCUBIT07;
		}
		if ((noteval & GEVCUBIT08) != 0) 
		{ // CAN:  
			GevcuEvents_08(pcf);
			noteuse |= GEVCUBIT08;
		}
		if ((noteval & GEVCUBIT09) != 0) 
		{ // CAN:  
			GevcuEvents_09(pcf);
			noteuse |= GEVCUBIT09;
		}
		if ((noteval & GEVCUBIT10) != 0) 
		{ // CAN:  
			GevcuEvents_10(pcf);
			noteuse |= GEVCUBIT10;
		}
		if ((noteval & GEVCUBIT11) != 0) 
		{ // CAN:  
			GevcuEvents_11(pcf);
			noteuse |= GEVCUBIT11;
		}
		if ((noteval & GEVCUBIT12) != 0) 
		{ // CAN:  
			GevcuEvents_12(pcf);
			noteuse |= GEVCUBIT12;
		}
		if ((noteval & GEVCUBIT13) != 0) 
		{ // CAN:  
			GevcuEvents_13(pcf);
			noteuse |= GEVCUBIT13;
		}
		if ((noteval & GEVCUBIT14) != 0) 
		{ // CAN:  
			GevcuEvents_14(pcf);
			noteuse |= GEVCUBIT14;
		}
  /* ========= States =============================== */

		switch (pcf->state)
		{

		default:
			break;
		}
  /* ========= Update outputs ======================= */
		GevcuUpdates(pcf);
  }
}

