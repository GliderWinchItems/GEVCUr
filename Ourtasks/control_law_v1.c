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
	clv1.kp = 1.0f;  		// Proportional constant
	clv1.ki = 0.01f; 		// Integral constant
	clv1.fllspd = 1500.0f;	//	100% control lever desired speed magnitude
	clv1.clpi = 10.0f;		//	Integrator clipping level
	clv1.clpcp = 300.0f;		//	Command clipping level positive
	clv1.clpcn = -300.0f;	//	Command clipping level negative

	clv1.spderr   = 0;
	clv1.dsrdspd  = 0;
	clv1.intgrtr  = 0;
	
	/* Initialize DMOC that is in SPEED mode. */
	dmoc_control_initSPEED();
	return;
}

/* *************************************************************************
 * void control_law_v1_reset(void);
 * @brief	: Reset
 * *************************************************************************/
void control_law_v1_reset(void)
{
	clv1.intgrtr   = 0;
	dmocctl[DMOC_SPEED].ftorquereq = 0.0f;
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
	if (init_flag == 0) control_law_v1_init();

	//	Compute desred speed based on control lever and PB conditons
	/* Press pushbutton for direction reversal */
	clv1.dsrdspd = 0.01f * clfunc.curpos * clv1.fllspd;	//	Desired speed magnitude
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
	clv1.intgrtr += clv1.spderr * clv1.ki;
	if (clv1.intgrtr > clv1.clpi) 
	{
		clv1.intgrtr = clv1.clpi;
	}
	else if (clv1.intgrtr < -clv1.clpi)
	{
		clv1.intgrtr = -clv1.clpi;
	}

	//	Compute and limit torque command
	pdmocctl->ftorquereq = clv1.spderr * clv1.kp + clv1.intgrtr;
	if (pdmocctl->ftorquereq > clv1.clpcp) 
	{
		pdmocctl->ftorquereq = clv1.clpcp;
	}
	else if (pdmocctl->ftorquereq < clv1.clpcn)
	{
		pdmocctl->ftorquereq = clv1.clpcn;
	}

	/* Update LED state. */
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
