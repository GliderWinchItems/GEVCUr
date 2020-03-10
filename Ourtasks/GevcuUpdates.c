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

#include "morse.h"

/* From 'main.c' */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;

/* *************************************************************************
 * void GevcuUpdates(void);
 * @brief	: Update outputs based on bits set
 * *************************************************************************/
void GevcuUpdates(void)
{
	/* Contactor keepalive/command msg sending. */
	contactor_control_CANsend();
	
	/* DMOC CAN msg sending. */
	// pushbutton (0 or 1) * percent ajustment (0.01) * Current CL position (0-100.0)
   //   * Maximum torque command value (e.g. +/- 300 Nm).
	dmocctl[0].torquereq = dmocctl[0].pbctl * 0.01f * clfunc.curpos * dmocctl[0].maxtorque; 
	dmoc_control_CANsend(&dmocctl[0]); // DMOC #1

	/* Keepalive and torque command for DMOC */
	dmoc_control_time(&dmocctl[0], gevcufunction.swtim1ctr);

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

