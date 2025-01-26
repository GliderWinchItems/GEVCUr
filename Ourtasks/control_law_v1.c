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
#include "dmoc_control.h"

struct CTLLAWPILOOP clv1;

static uint8_t init_flag = 0; // Bootup one-time init

/* *************************************************************************
 * void control_law_v1_init(void);
 * @brief	: Load parameters
 * *************************************************************************/
void control_law_v1_init(void)
{
	init_flag = 1; // Bootup one-time init

	/* Load parameters and initialize variables. */
	/* See: struct CTLLAWPILOOP in dmoc_control.h. */
	clv1.spderr   = 0;
	clv1.dsrdspd  = 0;
	clv1.intgrtr  = 0;
	
	/* SPEED mode. */
	dmoc_control_init(&dmocctl[0], DMOC_MODESPEED);
	return;
}

/* *************************************************************************
 * void control_law_v1_reset(void);
 * @brief	: Reset
 * *************************************************************************/
void control_law_v1_reset(void)
{
	clv1.intgrtr   = 0;
//$	dmocctl[DMOC_SPEED].ftorquereq = 0.0f;
	return;
}

/* *************************************************************************
 * void control_law_v1_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v1_calc(struct DMOCCTL* pdmocctl)
{
	struct DM1_LC* p = &pdmocctl->lc; // Pointer to fixed parameters (dm1_idx_v_struct.[ch])

	/* Init parameters automatically on bootup. */
	if (init_flag == 0) control_law_v1_init();

	//	Compute desred speed based on control lever and PB conditons
	/* Press pushbutton for direction reversal */
	clv1.dsrdspd = 0.01f * clfunc.curpos * p->fllspd;	//	Desired speed magnitude
	if (gevcufunction.psw[PSW_ZODOMTR]->db_on == SW_CLOSED)
	{ 
		clv1.dsrdspd = -clv1.dsrdspd;
		led_retrieve.mode = LED_ON;
	}
	else
	{
		led_retrieve.mode = LED_OFF;
	}

	//	Compute speed error
	clv1.spderr = clv1.dsrdspd - pdmocctl->speedact;

	//	Update integrator and clp if needed
	clv1.intgrtr += clv1.spderr * p->ki;
	if (clv1.intgrtr > p->clpi) 
	{
		clv1.intgrtr = p->clpi;
	}
	else if (clv1.intgrtr < -p->clpi)
	{
		clv1.intgrtr = -p->clpi;
	}

	//	Compute and limit torque command
	pdmocctl->ftorquereq = clv1.spderr * p->kp + clv1.intgrtr;
	if (pdmocctl->ftorquereq > p->clpc) 
	{
		pdmocctl->ftorquereq = p->clpc;
	}
	else if (pdmocctl->ftorquereq < -p->clpc)
	{
		pdmocctl->ftorquereq = -p->clpc;
	}

	/* Update LED state. */
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
