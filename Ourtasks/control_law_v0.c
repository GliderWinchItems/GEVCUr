/******************************************************************************
* File Name          : control_law_v0.c
* Date First Issued  : 03/19/2020
* Board              : DiscoveryF4
* Description        : Compute torque request for dmoc
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
#include "control_law_v0.h"

/* *************************************************************************
 * void control_law_v0_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v0_calc(struct DMOCCTL* pdmocctl)
{
	/* Press pushbutton for alternate defined torque. */
	if (gevcufunction.psw[PSW_ZODOMTR]->db_on == SW_CLOSED)
	{ 
		/* Pct (0.01) * CL position (0-100.0) * max torque (likely) negative (Nm) */
		pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->fmaxtorque_pbclosed;
		led_retrieve.mode = LED_ON;
	}
	else
	{
		/* Pct (0.01) * CL position (0-100.0) * max torque positive (Nm) */
		pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->fmaxtorque_pbopen;
		led_retrieve.mode = LED_OFF;
	}

	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
