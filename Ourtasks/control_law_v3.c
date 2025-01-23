/******************************************************************************
* File Name          : control_law_v0.c
* Date First Issued  : 03/19/2020
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

/* *************************************************************************
 * void control_law_v3_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v3_calc(struct DMOCCTL* pdmocctl)
{

	if (pdmocctl->speedact >= pdmocctl->lc.maxspeed_pos) 
	{ // Here at upper speed limit
		pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_neg;
		led_retrieve.mode = LED_OFF;
	}
	else
	{
		if (pdmocctl->speedact <= pdmocctl->lc.maxspeed_neg) 
		{
			pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_pos;
			led_retrieve.mode = LED_ON;
		}
	}
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
