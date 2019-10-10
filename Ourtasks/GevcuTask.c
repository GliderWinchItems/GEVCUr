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
	xTaskNotify(GevcuTaskHandle, CNCTBIT04, eSetBits);
	return;
}
/* *************************************************************************
 * void swtim2_callback(TimerHandle_t tm);
 * @brief	: Software timer 2 timeout callback
 * *************************************************************************/
static void swtim2_callback(TimerHandle_t tm)
{
	xTaskNotify(GevcuTaskHandle, CNCTBIT05, eSetBits);
	return;
}
/* *************************************************************************
 * void swtim3_callback(TimerHandle_t tm);
 * @brief	: Software timer 3 timeout callback
 * *************************************************************************/
static void swtim3_callback(TimerHandle_t tm)
{
	xTaskNotify(GevcuTaskHandle, CNCTBIT03, eSetBits);
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
 	osThreadDef(GevcuTask, StartGevcuTask, osPriorityNormal, 0, 128);
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
	pcf->prbcb3  = xSerialTaskRxAdduart(&huart3,0,CNCTBIT01,&noteval,2,48,0,0);
	if (pcf->prbcb3 == NULL) morse_trap(47);

	/* Init struct with working params */
	gevcu_idx_v_struct_hardcode_params(&gevcufunction.lc);

	/* Initialize working struc for GevcuTask. */
	extern struct ADCFUNCTION adc1;
	gevcu_func_init_init(pcf, &adc1);

	/* CAN hardware filter: restrict incoming to necessary CAN msgs. */
	gevcu_func_init_canfilter(pcf);
      
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
		if ((noteval & CNCTBIT00) != 0)
		{ // ADC readings ready
			GevcuEvents_00(pcf);
			noteuse |= CNCTBIT00;
		}
		if ((noteval & CNCTBIT01) != 0)
		{ // uart RX line ready
			GevcuEvents_01(pcf);
			noteuse |= CNCTBIT01;
		}
		if ((noteval & CNCTBIT02) != 0)
		{ // (spare)
			noteuse |= CNCTBIT02;
		}
		if ((noteval & CNCTBIT03) != 0)
		{ // TIMER3: uart RX keep alive timed out
			GevcuEvents_03(pcf);			
			noteuse |= CNCTBIT03;
		}
		if ((noteval & CNCTBIT04) != 0)
		{ // TIMER1: Command Keep Alive time out (periodic)
			GevcuEvents_04(pcf);
			noteuse |= CNCTBIT04;
		}
		if ((noteval & CNCTBIT05) != 0)
		{ // TIMER2: Multiple use Delay timed out
			GevcuEvents_05(pcf);
			noteuse |= CNCTBIT05;
		}
		if ((noteval & CNCTBIT06) != 0) 
		{ // CAN: cid_cmd_i 
			GevcuEvents_06(pcf);
			noteuse |= CNCTBIT06;
		}
		if ((noteval & CNCTBIT07) != 0) 
		{ // CAN: cid_keepalive_i received
			GevcuEvents_07(pcf);
			noteuse |= CNCTBIT07;
		}
		if ((noteval & CNCTBIT08) != 0) 
		{ // CAN: cid_gps_sync 
			GevcuEvents_08(pcf);
			noteuse |= CNCTBIT08;
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

