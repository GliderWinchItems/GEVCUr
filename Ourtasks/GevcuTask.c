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
#include "calib_control_lever.h"
#include "contactor_control.h"
#include "dmoc_control.h"

#include "main.h"
#include "morse.h"
#include "getserialbuf.h"
#include "yprintf.h"
#include "SwitchTask.h"
#include "shiftregbits.h"


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
 * osThreadId xGevcuTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: GevcuTaskHandle
 * *************************************************************************/
osThreadId xGevcuTaskCreate(uint32_t taskpriority)
{
 	osThreadDef(GevcuTask, StartGevcuTask, osPriorityNormal, 0, (312-14));
	GevcuTaskHandle = osThreadCreate(osThread(GevcuTask), NULL);
	vTaskPrioritySet( GevcuTaskHandle, taskpriority );
	return GevcuTaskHandle;
}
/* *************************************************************************
 * void StartGevcuTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
struct SWITCHPTR* pb_reversetorq; // Debugging

void StartGevcuTask(void const * argument)
{
taskflags |= TSKBITGevcuTask ;
	/* A notification copies the internal notification word to this. */
	uint32_t noteval = 0;    // Receives notification word upon an API notify
	uint32_t noteuse = 0xffffffff;

	/* Setup serial receive for uart */
	/* Get buffer control block for incoming uart lines. */
	// 2 line buffers of 48 chars, no dma buff, char-by-char line mode
//	gevcufunction.prbcb3  = xSerialTaskRxAdduart(&huart3,0,GEVCUBIT01,&noteval,2,48,0,0);
//	if (gevcufunction.prbcb3 == NULL) morse_trap(47);

	/* Init struct with working params */
	gevcu_idx_v_struct_hardcode_params(&gevcufunction.lc);

	/* Initialize working struc for GevcuTask. */
	extern struct ADCFUNCTION adc1;
	gevcu_func_init_init(&gevcufunction, &adc1);

	/* CAN hardware filter: restrict incoming to necessary CAN msgs. */
	// If gateway is included, then all CAN msgs are needed.
#ifndef GATEWAYTASKINCLUDED
	gevcu_func_init_canfilter(&gevcufunction);
#endif

	/* Instantiate switches used by this task.        */
	/* Pointer returned points to struct with status. */
	// Pushbutton to reverse torque
	struct SWITCHPTR* psw_z_tension = switch_pb_add(
		NULL,            /* task handle = this task    */
		GEVCUBIT03,      /* Task notification bit      */
		CP_REVERSETORQ,  /* See shiftregbits.h.        */
		0,               /* Not a switch pair          */
	 	SW_WAITDB,       /* Recognition mode           */
	 	3,               /* Debounce ct: closing       */
	   1);              /* Debounce ct: opening       */    
	if (psw_z_tension == NULL) morse_trap(57); // (Not needed)

pb_reversetorq = psw_z_tension; // Debugging aid

	/* Create timer Auto-reload/periodic */
	gevcufunction.swtimer1 = xTimerCreate("swtim1",gevcufunction.ka_k,pdTRUE,\
		(void *) 0, swtim1_callback);
	if (gevcufunction.swtimer1 == NULL) {morse_trap(40);}

	/* Initial startup state */
	gevcufunction.state = GEVCU_INIT;

	/* Some initialization for contactor control. */
	contactor_control_init();

	/* Some initialization for first DMOC unit control. */
	dmoc_control_init(&dmocctl[0]);

	/* Start command/keep-alive timer */
	BaseType_t bret = xTimerReset(gevcufunction.swtimer1, 10);
	if (bret != pdPASS) {morse_trap(44);}

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
//			GevcuEvents_00(); // Skip and let ADCTask to the CL work
			noteuse |= GEVCUBIT00;
		}
		if ((noteval & GEVCUBIT01) != 0)
		{ // Switches changed to ACTIVE
//			GevcuEvents_01();
			noteuse |= GEVCUBIT01;
		}
		if ((noteval & GEVCUBIT02) != 0)
		{ // Switches changed to SAFE
//			GevcuEvents_02();
			noteuse |= GEVCUBIT02;
		}
		if ((noteval & GEVCUBIT03) != 0)
		{ // Torque reversal pushbutton
			GevcuEvents_03(psw_z_tension);			
			noteuse |= GEVCUBIT03;
		}
		if ((noteval & GEVCUBIT04) != 0)
		{ // TIMER1:  (periodic)
			GevcuEvents_04();
			noteuse |= GEVCUBIT04;
		}
		if ((noteval & GEVCUBIT05) != 0)
		{ // spare (notification not expected)
//			GevcuEvents_05();
			noteuse |= GEVCUBIT05;
		}
		if ((noteval & GEVCUBIT06) != 0) 
		{ // CAN:  
			GevcuEvents_06();
			noteuse |= GEVCUBIT06;
		}
		if ((noteval & GEVCUBIT07) != 0) 
		{ // CAN: 
			GevcuEvents_07();
			noteuse |= GEVCUBIT07;
		}
		if ((noteval & GEVCUBIT08) != 0) 
		{ // CAN:  cid_dmoc_actualtorq
			GevcuEvents_08();
			noteuse |= GEVCUBIT08;
		}
		if ((noteval & GEVCUBIT09) != 0) 
		{ // CAN:  cid_dmoc_speed
			GevcuEvents_09();
			noteuse |= GEVCUBIT09;
		}
		if ((noteval & GEVCUBIT10) != 0) 
		{ // CAN:  cid_dmoc_dqvoltamp (see * below)
			GevcuEvents_10();
			noteuse |= GEVCUBIT10;
		}
		if ((noteval & GEVCUBIT11) != 0) 
		{ // CAN: cid_dmoc_torque (see * below)
			GevcuEvents_11();
			noteuse |= GEVCUBIT11;
		}
		if ((noteval & GEVCUBIT12) != 0) 
		{ // CAN:  cid_dmoc_critical_f (see * below)
			GevcuEvents_12();
			noteuse |= GEVCUBIT12;
		}
		if ((noteval & GEVCUBIT13) != 0) 
		{ // CAN:  cid_dmoc_hv_status
			GevcuEvents_13();
			noteuse |= GEVCUBIT13;
		}
		if ((noteval & GEVCUBIT14) != 0) 
		{ // CAN:  cid_dmoc_hv_temps
			GevcuEvents_14();
			noteuse |= GEVCUBIT14;
		}
		if ((noteval & GEVCUBIT15) != 0) 
		{ // CAN:  cid_gevcur_keepalive_i (see * below)
			GevcuEvents_15();
			noteuse |= GEVCUBIT15;
		}
// * = gevcu_func_init.c: this CAN msg not initialized for a Mailbox */

  /* ========= States =============================== */

		switch (gevcufunction.state)
		{
		case GEVCU_INIT:
			break;

		default:
			break;
		}
  /* ========= Update outputs ======================= */
		GevcuUpdates();
  }
while(1==1);
}

