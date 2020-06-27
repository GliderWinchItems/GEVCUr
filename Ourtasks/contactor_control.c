/******************************************************************************
* File Name          : contactor_control.c
* Date First Issued  : 12/01/2019
* Board              : DiscoveryF4
* Description        : Control of contactor unit
*******************************************************************************/

#include "contactor_control.h"
#include "main.h"
#include "morse.h"
#include "contactor_control_msg.h"
#include "LcdTask.h"
#include "LcdmsgsetTask.h"
#include "lcdprintf.h"

extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block

struct CNTCTRCTL cntctrctl; // Contactor Control

/* ***********************************************************************************************************
 * void contactor_control_init(void);
 * @brief	: Prep for contactor handling
 ************************************************************************************************************* */
void contactor_control_init(void)
{
	cntctrctl.state    = CTL_INITTIM;
	cntctrctl.req      = CMDRESET;  // Start off disconnected
	cntctrctl.sendflag = 0;
	cntctrctl.responsectr = 0;    // No response counter
	cntctrctl.nrflag   = 1;  // No response state is not responding



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
static struct LCDMSGSET lcdi2cfunc2;
//                                                                             "12345678901234567890"
static void lcdi2cmsg2(union LCDSETVAR u){lcdi2cprintf(&punitd4x20,CNCTRLCDA,0,"CNTR NO RESPNSE%5d",u.u32);} 

void contactor_control_time(uint32_t ctr)
{
	cntctrctl.responsectr += 1;    // No response counter
	if (cntctrctl.responsectr >= CNCTR_NORESPONSECT)
	{ // Here, too many time ticks without a received msg.
	/* Set context for LCD line usage. */
		lcdcontext |= LCDX_CNTR; // Contactor faulted
		cntctrctl.nrflag = 1;
		cntctrctl.slow += 1;
		if (cntctrctl.slow >= CNCTR_SLOW)
		{ 
			cntctrctl.slow = 0;
			lcdi2cfunc2.ptr = lcdi2cmsg2;
			lcdi2cfunc2.u.u32 = cntctrctl.responsectr;
			xQueueSendToBack(LcdmsgsetTaskQHandle, &lcdi2cfunc2, 0);
		}
	}

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
 * void contactor_control_CANrcv(struct CANRCVBUF* pcan);
 * @brief	: Handle contactor command CAN msgs being received
 * @param	: pcan = pointer to CAN msg struct
 ************************************************************************************************************* */
static struct LCDMSGSET lcdi2cfunc1;
//                                                                             "12345678901234567890"
static void lcdi2cmsg1(union LCDSETVAR u){lcdi2cprintf(&punitd4x20,CNCTRLCDA,0,"CNTR CLEAR FAULT%4d",u.u32);} 
static void lcdi2cmsg3(union LCDSETVAR u){lcdi2cputs  (&punitd4x20,CNCTRLCDA,0,"                    ");} // Line of blanks

void contactor_control_CANrcv(struct CANRCVBUF* pcan)
{
	/* Any msg received resets no-response counter and msg. */
	if (cntctrctl.responsectr >= CNCTR_NORESPONSECT)
	{ // Here we were in a no-response situation
		lcdi2cfunc1.ptr = lcdi2cmsg3; // Clear NO RESPONSE msg
		xQueueSendToBack(LcdmsgsetTaskQHandle, &lcdi2cfunc1, 0);
	}
	cntctrctl.responsectr = 0; // Reset no-response counter
	cntctrctl.nrflag = 0;

	/* Update contactor msg payload command byte */
	cntctrctl.cmdrcv = pcan->cd.uc[0]; // Extract command byte from contactor

	uint32_t ctr = gevcufunction.swtim1ctr;

	/* Set context for LCD line usage. */
	if ((pcan->cd.uc[0] & 0xf) == FAULTED) 
		lcdcontext |= LCDX_CNTR; // Contactor faulted
	else
		lcdcontext &= ~LCDX_CNTR; // Not faulted

	/* Check requested state: CONNECT or DISCONNECT. */
	if ((cntctrctl.req & CMDRESET) != 0)
	{ // Here, do a reset which does the disconnecting process. */
		cntctrctl.cmdsend  = CMDRESET;
		cntctrctl.nextctr  = ctr + CNCTR_KAQUICKTIC; // Time to send next KA msg
		cntctrctl.state    = CTL_CLEARFAULT; // Start connect sequence when CMDCONNECT given
		return;	
	}
	/* Here, not CMDRESET means CONNECT. */

	switch(cntctrctl.state)
	{
	case CTL_CLEARFAULT:
		contactor_control_msg(pcan); // LCD display
		if (pcan->cd.uc[1] != 0)
		{ // A fault is showing
			cntctrctl.ctr += 1;
			if (cntctrctl.ctr > 4)
			{ // 
				cntctrctl.ctr = 0;
				lcdi2cfunc1.ptr = lcdi2cmsg1;
				lcdi2cfunc1.u.u32 = cntctrctl.ctr;
				xQueueSendToBack(LcdmsgsetTaskQHandle, &lcdi2cfunc1, 0);
				break;
			}
		}
		/* Leave in disconnected state until someone calls for connect. */
		cntctrctl.cmdsend  = CMDDISCONNECT;
		cntctrctl.ctr = 0;
		cntctrctl.state = CTL_DISCONNECTED;
		break;

	case CTL_CONNECTING:
		cntctrctl.cmdsend  = CMDCONNECT;
		contactor_control_msg(pcan); // LCD display
		if ((pcan->cd.uc[0] & 0xf) != CONNECTED)
		{
			cntctrctl.ctr += 1;
			if (cntctrctl.ctr > 300)
			{ // It is taking too long. Re-start
				cntctrctl.state = CTL_INITTIM;
				break;
			}
			break;
		}
		cntctrctl.state = CTL_CONNECTED1;
		break;

	case CTL_CONNECTED1: // Last LCD msg display
		contactor_control_msg(pcan); // LCD display
		cntctrctl.state = CTL_CONNECTED;
		break;

	case CTL_CONNECTED:
		if ((pcan->cd.uc[0] & 0xf) != DISCONNECTED)
		{
			contactor_control_msg(pcan); // LCD display
			cntctrctl.state = CTL_DISCONNECTED;
		}
		break;

	case CTL_DISCONNECTED:
		/* Wait for someone to call for connect. */
		if (cntctrctl.cmdsend == CMDCONNECT)
		{
			cntctrctl.state = CTL_CONNECTING;
		}
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

