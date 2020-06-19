/******************************************************************************
* File Name          : contactor_control_msg.c
* Date First Issued  : 02/25/2020
* Board              : DiscoveryF4
* Description        : Status msgs received from contactor unit
*******************************************************************************/

#include <string.h>
#include "contactor_control.h"
#include "morse.h"
#include "calib_control_lever.h"
#include "GevcuTask.h"
#include "getserialbuf.h"
#include "yprintf.h"
#include "lcdprintf.h"
#include "LcdTask.h"
#include "LcdmsgsetTask.h"

enum CONTACTOR_FAULTCODE
{
	NOFAULT,
	BATTERYLOW,
	CONTACTOR1_OFF_AUX1_ON,
	CONTACTOR2_OFF_AUX2_ON,
	CONTACTOR1_ON_AUX1_OFF,
	CONTACTOR2_ON_AUX2_OFF,
	CONTACTOR1_DOES_NOT_APPEAR_CLOSED,
   PRECHGVOLT_NOTREACHED,
	CONTACTOR1_CLOSED_VOLTSTOOBIG,
	CONTACTOR2_CLOSED_VOLTSTOOBIG,
	KEEP_ALIVE_TIMER_TIMEOUT,
	NO_UART3_HV_READINGS,
	HE_AUTO_ZERO_TOLERANCE_ERR,
};
extern struct LCDI2C_UNIT* punitd4x20; // Pointer LCDI2C 4x20 unit

/******************************************************************************
 * void contactor_control_msg(struct CANRCVBUF* p);
 * @brief 	: Send CAN msgs
 * @param	: p = pointer to CAN msg response from contactor
*******************************************************************************/
void contactor_control_msg(struct CANRCVBUF* p)
{
	/* calib_control_lever handles LCD startup sequence. */
	// 0 = LCD not initialized; 1 = LCD OK to use
	if (flag_cllcdrdy == 0)
		return;

	if (p->dlc != 3)
	{ 
//                                         "12345678901234567890"
		lcdi2cputs(&punitd4x20,CNCTRLCDB,0,"CNTR   ER dlc != 3  ");
		lcdcontext |= LCDX_CNTR; // Set LCD context flag to override other msgs.
		return;
	}

	// Primary state code, Substate code
//	lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0," : %2i %2i ",(p->cd.uc[0] & 0xf),(p->cd.uc[2] & 0xf));

 if (p->cd.uc[1] == 0)
 {	// Here, no critical faults
	switch (p->cd.uc[0] & 0xf)
	{
	case	DISCONNECTED:   /*  0 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  DISCONNECTED  "); 
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  DISCONNECTED  "); 
		break;
	case	CONNECTING:     /*  1 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  CONNECTING    ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  CONNECTING    ");
		break;	
	case	CONNECTED:      /*  2 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  CONNECTED     ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  CONNECTED     ");
		break;	
	case	FAULTING:       /*  3 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  FAULTING      ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  FAULTING      ");
		break;	
	case	FAULTED:        /*  4 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  FAULTED       ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  FAULTED       ");
		break;	
	case	RESETTING:      /*  5 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  RESETTING     ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  RESETTING     ");
		break;	
	case	DISCONNECTING:  /*  6 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  DISCONNECTING ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  DISCONNECTING ");
		break;	
	case	OTOSETTLING:    /*  7 */
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  OTOSETTLNG    "); 
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  OTOSETTLNG    "); 
		break;
	default:
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd1,CNCTRLCDA,0,"CNTR  RCV BAD CODE");
		lcdi2cputs         (&punitd4x20,CNCTRLCDA,0,"CNTR  RCV BAD CODE");
	}
 }
 else
 { // Here, fault code
	switch (p->cd.uc[1])
	{
	case NOFAULT: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"CONTACTOR NO_FAULT  ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"CONTACTOR NO_FAULT  ");
		break;
	case BATTERYLOW: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"BATTERYLOW          ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"BATTERYLOW          ");
		break;
	case CONTACTOR1_OFF_AUX1_ON: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR1_OFF_AUX1_ON");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR1_OFF_AUX1_ON");
		break;
	case CONTACTOR2_OFF_AUX2_ON: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR2_OFF_AUX2_ON");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR2_OFF_AUX2_ON");
		break;
	case CONTACTOR1_ON_AUX1_OFF: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR1_ON_AUX1_OFF");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR1_ON_AUX1_OFF");
		break;
	case CONTACTOR2_ON_AUX2_OFF: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR2_ON_AUX2_OFF");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR2_ON_AUX2_OFF");
		break;
	case CONTACTOR1_DOES_NOT_APPEAR_CLOSED: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR1_NOT_CLOSED?");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR1_NOT_CLOSED?");
		break;
	case PRECHGVOLT_NOTREACHED: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X PRECHGV_NOTREACHED");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X PRECHGV_NOTREACHED");
		break;
	case CONTACTOR1_CLOSED_VOLTSTOOBIG: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR1_BIG VOLTS");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR1_BIG VOLTS");
		break;
	case CONTACTOR2_CLOSED_VOLTSTOOBIG: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CNTCR2_BIG VOLTS");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CNTCR2_BIG VOLTS");
		break;
	case KEEP_ALIVE_TIMER_TIMEOUT: 
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X KEEP_ALIVE_TIMEOUT");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X KEEP_ALIVE_TIMEOUT");
		break;
	case NO_UART3_HV_READINGS:
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X UART3_HV_timed out");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X UART3_HV_timed out");
		break;
	case 	HE_AUTO_ZERO_TOLERANCE_ERR:
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X HE_AUTO_ZERO_ERR  ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X HE_AUTO_ZERO_ERR  ");
		break;
	default:
//                                                     12345678901234567890              
		// lcdprintf(&gevcufunction.pbuflcd2,CNCTRLCDB,0,"X CODE BAD CODE     ");
		lcdi2cputs         (&punitd4x20,CNCTRLCDB,0,"X CODE BAD CODE     ");
	}
 }
 return;
}

