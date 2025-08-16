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

static uint8_t direction;

/* *************************************************************************
 * void control_law_v3_calc(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/
void control_law_v3_calc(struct DMOCCTL* pdmocctl)
{
	
   if (clfunc.curpos == 0.0)
   {
      direction = 0;		//  initialize starting direction
		led_retrieve.mode = LED_OFF;
      pdmocctl->ftorquereq = 0.0;
      return;  
   }
         
    	if (pdmocctl->speedact >= pdmocctl->lc.upper_speed_lmt) 
	{ 	// Here at or above upper speed limit
		direction = 0;
	}
	else if (pdmocctl->speedact <= pdmocctl->lc.lower_speed_lmt) 
   { // Here at or below lower speed limit
		direction = 1;
   }
        

	if (direction == 0)
	{ 	//	here reverse acceleration
		led_retrieve.mode = LED_ON;
		if (pdmocctl->speedact > pdmocctl->lc.rev_kink_speed)
		{
			/* Pct (0.01) * CL position (0-100.0) * max negative torque 1 (Nm) */
			pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_neg_1;
		}
		else
		{
			/* Pct (0.01) * CL position (0-100.0) * max negative torque 2 (Nm) */
			pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_neg_2;	
		}                
	}
	else
	{	//	here forward acceleration
		led_retrieve.mode = LED_OFF;
		if (pdmocctl->speedact < pdmocctl->lc.fwd_kink_speed)
		{
			/* Pct (0.01) * CL position (0-100.0) * max negative torque 1 (Nm) */
			pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_pos_1;
		}
		else
		{
			/* Pct (0.01) * CL position (0-100.0) * max negative torque 2 (Nm) */
			pdmocctl->ftorquereq = 0.01f * clfunc.curpos * pdmocctl->lc.fmaxtorque_pos_2;	
		}         
	}
	xQueueSendToBack(LEDTaskQHandle,&led_retrieve,portMAX_DELAY);
	return;
}
