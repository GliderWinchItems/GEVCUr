/******************************************************************************
* File Name          : GevcuUpdates.c
* Date First Issued  : 07/02/2019
* Description        : Update outputs in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"

#include "GevcuTask.h"
#include "gevcu_idx_v_struct.h"
#include "CanTask.h"
#include "gevcu_msgs.h"
#include "contactor_control.h"
#include "dmoc_control.h"
#include "calib_control_lever.h"
#include "control_law_v1.h"

#include "morse.h"

/* From 'main.c' */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

static void payloadfloat(uint8_t *po, float f)
{
	union FFUI
	{
		float f;
		uint8_t u8[4];
	}ffui;
	ffui.f = f;

	*(po + 0) = ffui.u8[0];
	*(po + 1) = ffui.u8[1];
	*(po + 2) = ffui.u8[2];
	*(po + 3) = ffui.u8[3];
	return;
}

/* *************************************************************************
 * void GevcuUpdates(void);
 * @brief	: Update outputs based on bits set
 * *************************************************************************/
void GevcuUpdates(void)
{
	/* Contactor keepalive/command msg sending. */
	contactor_control_CANsend();
	
	/* DMOC CAN msg sending. */
	if (gevcufunction.state == GEVCU_ARM)
	{
		dmocctl[DMOC_SPEED].dmocopstate = DMOC_ENABLE;
	}
	else
	{
		dmocctl[DMOC_SPEED].dmocopstate = DMOC_DISABLED;
	}

	/* sendflag is set in GevcuEvents with timer tick call to dmoc_control_time. */
	if (dmocctl[DMOC_SPEED].sendflag != 0)
	{ // Here dmoc CMD1,2,3 are scheduled to be sent
			// Queue CAN msg with desired speed
		// Load desired speed (float) into payload starting at [0] 
		payloadfloat(&gevcufunction.canmsg[CID_GEVCUR_CTL_LAWV1].can.cd.uc[0],clv1.dsrdspd);
		// Load control law integrator value
		payloadfloat(&gevcufunction.canmsg[CID_GEVCUR_CTL_LAWV1].can.cd.uc[4],clv1.intgrtr);
		// Queue CAN msg for sending
		xQueueSendToBack(CanTxQHandle,&gevcufunction.canmsg[CID_GEVCUR_CTL_LAWV1],4);
	}

	/* Send required suite of three dmoc command msgs, and reset send flag. */
	dmoc_control_CANsend(&dmocctl[DMOC_SPEED]); 

	/* Queue GEVCUr keep-alive status CAN msg */
	if ((gevcufunction.outstat & CNCTOUT05KA) != 0)
	{
		gevcufunction.outstat &= ~CNCTOUT05KA;	
	}

	/* Reset new & various flags. */
	gevcufunction.evstat &= ~(
		EVSWTIM1TICK | /* Timer tick */
		EVNEWADC       /* new ADC readings */
		);

	return;
}

	
