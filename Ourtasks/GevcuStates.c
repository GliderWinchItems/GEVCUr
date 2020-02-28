/******************************************************************************
* File Name          : GevcuStates.c
* Date First Issued  : 07/01/2019
* Description        : States in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"
#include "LEDTask.h"

#include "GevcuEvents.h"
#include "GevcuStates.h"
#include "GevcuTask.h"
#include "calib_control_lever.h"
#include "contactor_control.h"
#include "yprintf.h"
#include "lcdprintf.h"

#include "gevcu_idx_v_struct.h"
#include "morse.h"
#include "adcparamsinit.h"

#define GEVCULCDMSGDELAY 32 // Minimum number of time ticks between LCD msgs
#define GEVCULCDMSGLONG (128*30) // Very long delay
static uint32_t gevcustates_timx;

enum GEVCU_INIT_SUBSTATEA
{
	GISA_OTO,  
	GISA_WAIT,
};

/* *************************************************************************
 * void GevcuStates_GEVCU_INIT(void);
 * @brief	: Initialization sequence: One Time Only
 * *************************************************************************/
void GevcuStates_GEVCU_INIT(void)
{	
	struct SWITCHPTR* p;

	switch (gevcufunction.substateA)
	{
	case GISA_OTO: // Cycle Safe/Active sw.
		if (flag_clcalibed == 0) 
			break;
//                                                  12345678901234567890             
		lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_INT           ");
		gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGDELAY;


		/* Update LED with SAFE/ACTIVE switch status. */
		p = gevcufunction.psw[PSW_PR_SAFE];
		gevcufunction.safesw_prev = p->db_on;
		switch (p->db_on)
		{
		case 1:
	      led_safe.mode = 0; // LED_SAFE off
			break;
			
		case 2:
	      led_safe.mode = 1; // LED_SAFE on
			break;
		}
		xQueueSendToBack(LEDTaskQHandle,&led_safe,portMAX_DELAY);

		gevcufunction.substateA = GISA_WAIT;
		break;

	case GISA_WAIT: // More OTO to do here?
		if (gevcufunction.psw[PSW_PR_SAFE]->db_on == SWP_OPEN )
		{ // Here SAFE/ACTIVE switch is in ACTIVE position
			if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
			{ // Sufficient delay between LCD msgs.
		//                                                  12345678901234567890
				lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"SWITCH TO SAFE      ");
				gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGDELAY;
			}
			break;
		}

		/* Go into safe mode,. */
		gevcufunction.state = GEVCU_SAFE_TRANSITION;
		break;

		default:
			break;
	}
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_SAFE_TRANSITION(void);
 * @brief	: Peace and quiet, waiting for hapless Op.
 * *************************************************************************/
void GevcuStates_GEVCU_SAFE_TRANSITION(void)
{
	if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
	{ // Sufficient delay between LCD msgs.
//                                                  12345678901234567890
		lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_SAFE_TRANSITIO");
		gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGLONG;

	}
      led_safe.mode    = LED_BLINKFAST; // LED_SAFE blinking
		led_arm_pb.mode  = LED_OFF; // ARM Pushbutton LED
		led_arm.mode     = LED_OFF; // ARM LED
		led_prep_pb.mode = LED_OFF; // PREP Pushbutton LED
		led_prep.mode    = LED_OFF; // PREP LED
	xQueueSendToBack(LEDTaskQHandle, &led_safe   ,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_arm_pb ,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_arm    ,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_prep_pb,portMAX_DELAY);
	xQueueSendToBack(LEDTaskQHandle, &led_prep   ,portMAX_DELAY);

	/* Request contactor to DISCONNECT. */
	cntctrctl.req = CMDRESET;

//#define DEHRIGTEST
#ifndef DEHRIGTEST
	/* Wait until contactor shows DISCONNECTED state. */
	if ((cntctrctl.cmdrcv & 0xf) != DISCONNECTED)
	{ // LCD msg here?
		return;
	}
#endif

   led_safe.mode    = LED_ON;
	xQueueSendToBack(LEDTaskQHandle, &led_safe   ,portMAX_DELAY);

	gevcufunction.state = GEVCU_SAFE;
	gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGDELAY;
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_SAFE(void);
 * @brief	: Peace and quiet, waiting for hapless Op.
 * *************************************************************************/
void GevcuStates_GEVCU_SAFE(void)
{
	if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
	{ // Sufficient delay between LCD msgs.
		gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGLONG;
//                                                  12345678901234567890
		lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_SAFE          ");
	}
		
	if (gevcufunction.psw[PSW_PR_SAFE]->db_on == SWP_OPEN )
	{ // Here SAFE/ACTIVE switch is in ACTIVE position
		gevcufunction.state = GEVCU_ACTIVE_TRANSITION;

		led_safe.mode = LED_BLINKFAST; // LED_SAFE fast blink mode
		xQueueSendToBack(LEDTaskQHandle,&led_safe,portMAX_DELAY);

		/* Request contactor to CONNECT. */
		cntctrctl.req = CMDCONNECT;
		gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGDELAY;
		return;
	}
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_ACTIVE_TRANSITION(void);
 * @brief	: Contactor & DMOC are ready. Keep fingers to yourself.
 * *************************************************************************/
void GevcuStates_GEVCU_ACTIVE_TRANSITION(void)
{
	if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
	{ // Sufficient delay between LCD msgs.
//                                                  12345678901234567890
		lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ACTIVE_TRANSIT");
		gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGLONG;
	}

#ifndef DEHRIGTEST
	/* Wait for CONNECTED. */
	if ((cntctrctl.cmdrcv & 0xf) != CONNECTED)
	{ // Put a stalled loop timeout here?
		cntctrctl.req = CMDCONNECT;
		return;
	}

#endif

	/* Contactor connected. */
	led_safe.mode   = LED_OFF; // LED_SAFE off
	xQueueSendToBack(LEDTaskQHandle,&led_safe,portMAX_DELAY);

	led_arm_pb.mode = LED_BLINKFAST; // ARM Pushbutton LED fast blink mode
	xQueueSendToBack(LEDTaskQHandle,&led_arm_pb,portMAX_DELAY);

	gevcufunction.state = GEVCU_ACTIVE;
	gevcustates_timx = gevcufunction.swtim1ctr	+ GEVCULCDMSGDELAY;
	return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_ACTIVE(void);
 * @brief	: Contactor & DMOC are ready. Keep fingers to yourself.
 * *************************************************************************/
void GevcuStates_GEVCU_ACTIVE(void)
{
		if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
		{ // Sufficient delay between LCD msgs.
	//                                                  12345678901234567890
			lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ACTIVE        ");
			gevcustates_timx = gevcufunction.swtim1ctr + GEVCULCDMSGLONG;
		}

		/* Wait for ARM pushbutton to be pressed. */	
		if (gevcufunction.psw[PSW_PB_ARM]->db_on != SW_CLOSED)
			return;
	
		/* Here, ARM_PB pressed, requesting ARMed state. */
		led_arm_pb.mode = LED_ON; // ARM Pushbutton LED
		xQueueSendToBack(LEDTaskQHandle,&led_arm_pb,portMAX_DELAY);

		gevcufunction.state = GEVCU_ARM_TRANSITION;
		gevcustates_timx = gevcufunction.swtim1ctr	+ GEVCULCDMSGDELAY; // 
		return;
}
/* *************************************************************************
 * void GevcuStates_GEVCU_ARM_TRANSITION(void);
 * @brief	: Do everything needed to get into state
 * *************************************************************************/
void GevcuStates_GEVCU_ARM_TRANSITION(void)
{
//		if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
//		{ // Sufficient delay between LCD msgs.
//			gevcustates_timx = gevcufunction.swtim1ctr	+ GEVCULCDMSGDELAY; // 
	//                                                  12345678901234567890
//			lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ARM_TRANSITION");
//		}

		/* Make sure Op has CL in zero position. */
		if (clfunc.curpos > 0)
		{
			if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
			{ // Sufficient delay between LCD msgs.
				gevcustates_timx = gevcufunction.swtim1ctr	+ GEVCULCDMSGDELAY;
		//                                                  12345678901234567890
				lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"ARM: MOVE CL ZERO   ");
				return;
			}	
		}
		if ((int)(gevcustates_timx - gevcufunction.swtim1ctr) > 0)
		{ // Sufficient delay between LCD msgs.
			gevcustates_timx = gevcufunction.swtim1ctr	+ GEVCULCDMSGDELAY;
	//                                                  12345678901234567890
			lcdprintf(&gevcufunction.pbuflcd3,GEVCUTSK,0,"GEVCU_ARM           ");
		}

		led_arm.mode    = LED_ON; // ARM LED ON
		xQueueSendToBack(LEDTaskQHandle,&led_arm,portMAX_DELAY);

		gevcufunction.state = GEVCU_ARM;
		return;

}
/* *************************************************************************
 * void GevcuStates_GEVCU_ARM(void);
 * @brief	: Contactor & DMOC are ready. Keep fingers to yourself.
 * *************************************************************************/
void GevcuStates_GEVCU_ARM(void)
{
	if (gevcufunction.psw[PSW_ZODOMTR]->db_on == SW_CLOSED)
	{
		/* Set DMOC torque to CL scaled. */
		led_retrieve.mode = LED_ON;
	}
	else
	{
		/* Set DMOC torque to zero. */
		led_retrieve.mode = LED_OFF;	
	}
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}

