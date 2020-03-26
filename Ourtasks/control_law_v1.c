/******************************************************************************
* File Name          : control_law_v1.c
* Date First Issued  : 03/22/2020
* Board              : DiscoveryF4
* Description        : Compute torque request for dmoc--PI Loop
*******************************************************************************/
/*
Speed PI Loop

*/
#include <stdio.h>

#include "GevcuTask.h"
#include "GevcuEvents.h"
#include "calib_control_lever.h"
#include "spiserialparallelSW.h"
#include "LEDTask.h"
#include "control_law_v1.h"

static uint8_t init_flag = 0; // Bootup one-time init

/* *************************************************************************
 * void control_law_v1_init(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Load parameters
 * *************************************************************************/
void control_law_v1_init(struct DMOCCTL* pdmocctl)
{
	init_flag = 1; // Bootup one-time init

	/* Load parameters and initialize variables. */
	/* See: struct CTLLAWPILOOP in dmoc_control.h. */
	pdmocctl->pi.kp = 1.0;  // Proportional constant
	pdmocctl->pi.ki = 1E-3; // Integral constant

	pdmocctl->pi.zdiff = 0.0; // Differentiator

	return;
}

/* *************************************************************************
 * void control_law_v1_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v1_calc(struct DMOCCTL* pdmocctl)
{

	/* Init parameters automatically on bootup. */
	if (init_flag == 0) control_law_v1_init(pdmocctl);

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

	/* Update LED state. */
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
