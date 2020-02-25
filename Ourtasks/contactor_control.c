/******************************************************************************
* File Name          : contactor_control.c
* Date First Issued  : 12/01/2019
* Board              : DiscoveryF4
* Description        : Control of contactor unit
*******************************************************************************/

#include "contactor_control.h"
#include "main.h"
#include "morse.h"

/* Command request bits assignments. 
Sourced location: ../contactor/OurTasks/ContactorTask.h
*/
#define CMDCONNECT (1 << 7) // 1 = Connect requested; 0 = Disconnect requested
#define CMDRESET   (1 << 6) // 1 = Reset fault requested; 0 = no command

extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block

struct CNTCTRCTL cntctrctl; // Contactor ControlSCTH90N65G2V-7

/* ***********************************************************************************************************
 * void contactor_control_init(void);
 * @brief	: Prep for contactor handling
 ************************************************************************************************************* */
void contactor_control_init(void)
{
	cntctrctl.state    = CTL_INITTIM;
	cntctrctl.sendflag = 0;

	/* Initialize keepalive CAN msg. */
	cntctrctl.canka.pctl       = pctl0; // CAN control block ptr, from main.c
	cntctrctl.canka.maxretryct = 8;
	cntctrctl.canka.bits       = 0; // /NART
	cntctrctl.canka.can.id     = gevcufunction.lc.cid_cntctr_keepalive_i;
	cntctrctl.canka.can.dlc    = 1;
	return;
}
/* ***********************************************************************************************************
 * void contactor_control_time(uint32_t ctr);
 * @brief	: Timer input to state machine
 * @param	: ctr = sw1ctr time ticks
 ************************************************************************************************************* */
void contactor_control_time(uint32_t ctr)
{
	if (cntctrctl.state == CTL_INITTIM)
	{ // OTO 
		cntctrctl.nextctr  = ctr + CNCTR_KATICKS; // Time to send next KA msg
		cntctrctl.cmdsend  = CMDRESET;// We start by clearing any contactor fault	
		cntctrctl.ctr      = 0;       // Repetition counter
		cntctrctl.sendflag = 1;       // Trigger sending (during GevcuUpdates.c)
		cntctrctl.state = CTL_CLEARFAULT; // Msg will be to clear any faults
		return;
	}

	/* Keepalive timing, based on sw1tim time ticks (e.g. (1/128) ms). */
	if ((int)(cntctrctl.nextctr - ctr)  > 0) return;

	/* Set next ctr for next KA msg. */
	cntctrctl.nextctr = ctr + CNCTR_KATICKS; // Time to send next KA msg
	cntctrctl.sendflag = 1; // Send first msg

	return;
}
/* ***********************************************************************************************************
 * void contactor_control_CANrcv(uint32_t ctr, struct CANRCVBUF* pcan);
 * @brief	: Handle contactor command CAN msgs being received
 * @param	: ctr = sw1tim tick counter
 * @param	: pcan = pointer to CAN msg struct
 ************************************************************************************************************* */
void contactor_control_CANrcv(uint32_t ctr, struct CANRCVBUF* pcan)
{
	/* Update contactor msg payload command byte */
	cntctrctl.cmdrcv = pcan->cd.uc[0]; // Extract command byte from contactor

	switch(cntctrctl.state)
	{
	case CTL_CLEARFAULT:

		if (pcan->cd.uc[1] != 0)
		{ // A fault is showing
			cntctrctl.ctr += 1;
			if (cntctrctl.ctr < 4)
			{ // Try to clear it a bunch of times
				break;
			}
		// else Some sort of LCD msg?
		}
		cntctrctl.ctr = 0;
		cntctrctl.cmdsend  = CMDCONNECT;
		cntctrctl.state = CTL_CONNECTING;
		break;

	case CTL_CONNECTING:
		if (pcan->cd.uc[2] != 0)
		{
			cntctrctl.ctr += 1;
			if (cntctrctl.ctr < 30)
			{ // It is taking too long. Re-start
				cntctrctl.state = CTL_INITTIM;
				break;
			}
		}
		cntctrctl.state = CTL_CONNECTED;
		break;

	case CTL_CONNECTED:
		// Here no need to respond to received msgs.
		// 'contactor_control_time' will continue to send CONNECT msgs
		break;

	default:
		morse_trap(343); // Never should happen.
		break;
	}
	cntctrctl.nextctr = ctr + CNCTR_KAQUICKTIC; // Time to send next KA msg
	return;
}
/* ***********************************************************************************************************
 * void contactor_control_CANsend(void);
 * @brief	: Send CAN msg
 ************************************************************************************************************* */
void contactor_control_CANsend(void)
{
	if (cntctrctl.sendflag == 0) return;

	cntctrctl.sendflag = 0; // Reset flag
	
	/* Send contarctor keepalive/command CAN msg. */
	// Set command that has been setup above
	cntctrctl.canka.can.cd.uc[0] = cntctrctl.cmdsend;

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&cntctrctl.canka,portMAX_DELAY);
	
	return;
}

