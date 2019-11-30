/******************************************************************************
* File Name          : GevcuEvents.c
* Date First Issued  : 07/01/2019
* Description        : Events in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/
/*
The CL calibration and ADC->pct position is done via ADC new readings notifications.


*/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"

#include "gevcu_idx_v_struct.h"
#include "GevcuTask.h"
#include "morse.h"

#include "SerialTaskReceive.h"
#include "GevcuTask.h"
#include "can_iface.h"
#include "gevcu_cmd_msg.h"
#include "gevcu_msgs.h"
#include "MailboxTask.h"
#include "calib_control_lever.h"

#include "main.h"

/* *************************************************************************
 * void GevcuEvents_00(void);
 * @brief	: ADC readings available
 * *************************************************************************/
void GevcuEvents_00(void)
{
	gevcufunction.evstat |= EVNEWADC; // Show new readings ready

	/* Update Control Lever with new ADC readings (or do initial calib). */
	calib_control_lever();
	return;
}
/* *************************************************************************
 * void GevcuEvents_01(void);
 * @brief	: (spare)
 * *************************************************************************/
void GevcuEvents_01(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_02(void);
 * @brief	: (spare)
 * *************************************************************************/
void GevcuEvents_02(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_03(void);
 * @brief	: TIMER3: Software timer 3 timeout
 * *************************************************************************/
void GevcuEvents_03(void)
{  // Readings failed to come in before timer timed out.
	return;
}
/* *************************************************************************
 * void GevcuEvents_04(void);
 * @brief	: TIMER1: Software timer 1
 * *************************************************************************/
uint32_t dbgev04;

void GevcuEvents_04(void)
{
	gevcufunction.swtim1ctr += 1;
	gevcufunction.evstat |= EVSWTIM1TICK; // Timer tick

	/* Keepalive for contactor CAN msgs. */
	gevcufunction.cntctr_ka_ctr += 1;
	if (gevcufunction.cntctr_ka_ctr >= gevcufunction.cntctr_ka_to)
	{
		gevcufunction.cntctr_ka_ctr = 0; // Reset timeout counter
		gevcufunction.evstat |= EVCNTCTR; // Set keepalive trigger bit
	}

	return;
}
/* *************************************************************************
 * void GevcuEvents_05(void);
 * @brief	: TIMER2: Software timer 2
 * *************************************************************************/
void GevcuEvents_05(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_06(void);
 * @brief	: CAN: cid_gps_sync
 * *************************************************************************/
void GevcuEvents_06(void)
{
//	gevcu_cmd_msg_i(pcf); // Build and send CAN msg with data requested
	return;
}
/* *************************************************************************
 * void GevcuEvents_07(void);
 * @brief	: CAN: cid_cntctr_keepalive_r
 * *************************************************************************/
void GevcuEvents_07(void)
{
	return;
}	
/* *************************************************************************
 * void GevcuEvents_08(void);
 * @brief	: CAN: cid_dmoc_actualtorq
 * *************************************************************************/
void GevcuEvents_08(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_09(void);
 * @brief	: CAN: cid_dmoc_speed
 * *************************************************************************/
void GevcuEvents_09(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_10(void);
 * @brief	: CAN: cid_dmoc_dqvoltamp
 * *************************************************************************/
void GevcuEvents_10(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_11(void);
 * @brief	: CAN: cid_dmoc_torque
 * *************************************************************************/
void GevcuEvents_11(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_12(void);
 * @brief	: CAN: cid_dmoc_critical_f
 * *************************************************************************/
void GevcuEvents_12(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_13(void);
 * @brief	: CAN: cid_dmoc_hv_status
 * *************************************************************************/
void GevcuEvents_13(void)
{
	return;
}
/* *************************************************************************
 * void GevcuEvents_14(void);
 * @brief	: CAN: cid_dmoc_hv_temps
 * *************************************************************************/
void GevcuEvents_14(void)
{
	return;
}

