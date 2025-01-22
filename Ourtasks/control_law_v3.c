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
	struct DM1_LC* p = &dmoctl.lc;

	if (pdmocctl->speedact >= p->maxspeed_pos) 
	{ // Here at upper speed limit
		pdmocctl->ftorquereq = 0.01f * clfunc.curpos * p->maxtorque_neg;
		led_retrieve.mode = LED_OFF;
	}
	else
	{
		if (pdmocctl->speedact <= p->maxspeed_neg) 
		{
			pdmocctl->ftorquereq = 0.01f * clfunc.curpos * p->maxtorque_pos;
			led_retrieve.mode = LED_ON;
		}
	}
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
