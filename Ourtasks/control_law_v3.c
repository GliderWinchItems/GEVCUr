/******************************************************************************
* File Name          : control_law_v3.c
* Date First Issued  : 01/27/2025
* Board              : DiscoveryF4
* Description        : Control for auto inertia testing
*******************************************************************************/
/*
torquereq = Simple scaling of Control Lever

*/
#include <stdio.h>

#include "GevcuTask.h"
#include "GevcuEvents.h"
#include "calib_control_lever.h"
#include "spiserialparallelSW.h"
#include "LEDTask.h"
#include "control_law_v3.h"

static uint8_t direction = 1;

/* *************************************************************************
 * void control_law_v3_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v3_calc(struct DMOCCTL* pdmocctl)
{
#if 0
	if (pdmocctl->speedact >= pdmocctl->lc.upper_speed_lmt) 
	{ // Here at or above upper speed limit
		direction = 0;
		led_retrieve.mode = LED_ON;
	}
	else if (pdmocctl->speedact <= pdmocctl->lc.lower_speed_lmt) 
        { // Here at or below lower speed limit
		direction = 1;
		led_retrieve.mode = LED_OFF;
	}
#endif

	if (direction == 0)
	{ 
		/* Pct (0.01) * CL position (0-100.0) * max negative torque negative (Nm) */
		pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_neg;
	}
	else
	{
		/* Pct (0.01) * CL position (0-100.0) * max positive torque  (Nm) */
		pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_pos;
	}
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
