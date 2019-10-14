/******************************************************************************
* File Name          : GevcuEvents.c
* Date First Issued  : 07/01/2019
* Description        : Events in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

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

/* *************************************************************************
 * void GevcuEvents_00(struct GEVCUFUNCTION* pcf);
 * @brief	: ADC readings available
 * *************************************************************************/
void GevcuEvents_00(struct GEVCUFUNCTION* pcf)
{
	pcf->evstat |= CNCTEVADC; // Show new readings ready
	return;
}

/* *************************************************************************
 * void GevcuEvents_01(struct GEVCUFUNCTION* pcf);
 * @brief	: usart RX line ready
 * *************************************************************************/
uint32_t dbgCE1;
void GevcuEvents_01(struct GEVCUFUNCTION* pcf)
{
dbgCE1 += 1;
	return;
}
/* *************************************************************************
 * void GevcuEvents_02(struct GEVCUFUNCTION* pcf);
 * @brief	: (spare)
 * *************************************************************************/
void GevcuEvents_02(struct GEVCUFUNCTION* pcf)
{
}
/* *************************************************************************
 * void GevcuEvents_03(struct GEVCUFUNCTION* pcf);
 * @brief	: TIMER3: uart RX keep alive failed
 * *************************************************************************/
void GevcuEvents_03(struct GEVCUFUNCTION* pcf)
{  // Readings failed to come in before timer timed out.
	return;
}
/* *************************************************************************
 * void GevcuEvents_04(struct GEVCUFUNCTION* pcf);
 * @brief	: TIMER1: Command Keep Alive failed (loss of command control)
 * *************************************************************************/
uint32_t dbgev04;

void GevcuEvents_04(struct GEVCUFUNCTION* pcf)
{
dbgev04 += 1;
	pcf->evstat |= CNCTEVTIMER1;	// Set to show that TIMER1 timed out


	/* Send with CAN id for heartbeat. */

	return;
}
/* *************************************************************************
 * void GevcuEvents_05(struct GEVCUFUNCTION* pcf);
 * @brief	: TIMER2: delay ended
 * *************************************************************************/
void GevcuEvents_05(struct GEVCUFUNCTION* pcf)
{
	pcf->evstat |= CNCTEVTIMER2;	// Set timeout bit 	
	return;
}
/* *************************************************************************
 * void GevcuEvents_06(struct GEVCUFUNCTION* pcf);
 * @brief	: CAN: cid_cmd_i (function/diagnostic command/poll)
 * *************************************************************************/
void GevcuEvents_06(struct GEVCUFUNCTION* pcf)
{
//	gevcu_cmd_msg_i(pcf); // Build and send CAN msg with data requested
	return;
}
/* *************************************************************************
 * void GevcuEvents_07(struct GEVCUFUNCTION* pcf);
 * @brief	: CAN: cid_keepalive_i 
 * *************************************************************************/
uint8_t dbgevcmd;

void GevcuEvents_07(struct GEVCUFUNCTION* pcf)
{
	BaseType_t bret = xTimerReset(pcf->swtimer1, 10);
	if (bret != pdPASS) {morse_trap(44);}

	pcf->outstat |=  CNCTOUT05KA;  // Output status bit: Show keep-alive
	pcf->evstat  &= ~CNCTEVTIMER1; // Reset timer1 keep-alive timed-out bit

	/* Incoming command byte with command bits */
	uint8_t cmd = pcf->pmbx_cid_keepalive_i->ncan.can.cd.uc[0];
dbgevcmd = cmd;

	/* Update connect request status bits */
	if ( (cmd & CMDCONNECT) != 0) // Command to connect
	{ // Here, request to connect
		pcf->evstat |= CNCTEVCMDCN;		
	}
	else
	{
		pcf->evstat &= ~CNCTEVCMDCN;		
	}
	/* Update reset status */
	if ( (cmd & CMDRESET ) != 0) // Command to reset
	{ // Here, request to reset
		pcf->evstat |= CNCTEVCMDRS;		
	}
	else
	{
		pcf->evstat &= ~CNCTEVCMDRS;		
	}
	return;
}	
/* *************************************************************************
 * void GevcuEvents_08(struct GEVCUFUNCTION* pcf);
 * @brief	: CAN: cid_gps_sync: send response CAN msgs
 * *************************************************************************/
uint32_t dbggpsflag;

void GevcuEvents_08(struct GEVCUFUNCTION* pcf)
{

/* Testing: use incoming gps msg to time defaultTask loop. */
struct CANRCVBUF* pcan = &pcf->pmbx_cid_gps_sync->ncan.can;
if (pcan->id == 0x00400000)
{
	if (pcan->cd.uc[0] == 0)
      dbggpsflag += 1;
}
	/* Send with regular polled CAN ID */
	return;
}
	
