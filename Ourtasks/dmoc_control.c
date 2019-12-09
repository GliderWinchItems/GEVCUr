/******************************************************************************
* File Name          : dmoc_control.c
* Date First Issued  : 12/04/2019
* Board              : DiscoveryF4
* Description        : Control of dmoc unit
*******************************************************************************/

#include "dmoc_control.h"
#include "main.h"
#include "morse.h"
#include "DMOCchecksum.h"
#include "paycnvt.h"

/* Name the indices to correspond to the GEVCU DMOC documentation. */
#define CMD1 0 
#define CMD2 1 
#define CMD3 2 

/* Command request bits assignments. 
Sourced location: ../dmoc/OurTasks/ContactorTask.h
*/

extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block

/* struct holds "everything" for each DMOC. */
struct DMOCCTL dmocctl[NUMDMOC]; // Array allows for multiple DMOCs

/* ***********************************************************************************************************
 * void dmoc_control_init(struct DMOCCTL* pdmocctl);
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Prep for dmoc handling
 ************************************************************************************************************* */
void dmoc_control_init(struct DMOCCTL* pdmocctl)
{
	int i,j;

	/* If values are semi-permanent in high-flash, then the following would be
      accomplished by copying from the high-flash areas. */

	/* NOTE--When there is more than one DMOC and the following parameters
      are applicable to both, then something like the following would be
      needed to be added, e.g.
      if (pdmocctl == &dmocctl[0])
      { // Initial DMOC #1
        ...
      }
		else
		{ // Here, initial "other" DMOC
        ... 
		}
		NOTE: for dynamometer testing one DMOC will be in torque mode
      and the other in speed mode. If the following initialization is used
      for both, the difference in modes will need to be set by "someone".
	*/

	pdmocctl->state     = DMOCINIT1; // Initial state
	pdmocctl->sendflag  = 0;
	pdmocctl->alive     = 0; // DMOC count increments by 2 & truncated
	pdmocctl->dmocstateact = DMOC_DISABLED;   // Assume initial state
	pdmocctl->dmocstatereq = DMOC_STANDBY;    // Requested startup state
	pdmocctl->mode         = DMOC_MODETORQUE; // Speed or Torque mode selection


	pdmocctl->speedreq      = 0;     // Requested speed
	pdmocctl->torquereq     = 0;     // Requested torque
	pdmocctl->torqueact     = 0;     // Torque actual (reported)
	pdmocctl->maxregenwatts = 60000; // ?
	pdmocctl->maxaccelwatts = 60000; // ?
	pdmocctl->torqueoffset  = 30000; // Torque command offset 
	pdmocctl->maxspeed      = 10000; // Max speed (signed)
	pdmocctl->maxtorque     = 29999; // Max torque (signed)

	pdmocctl->regencalc = 65000 - (pdmocctl->maxregenwatts / 4);
	pdmocctl->accelcalc = (pdmocctl->maxaccelwatts / 4);

	/* Load fixed data into three DMOC command CAN msgs. */
	for (i = 0; i < 3; i++)
	{
		pdmocctl->cmd[i].txqcan.pctl       = pctl0; // CAN1 control block ptr (from main.c)
		pdmocctl->cmd[i].txqcan.maxretryct = 8;
		pdmocctl->cmd[i].txqcan.bits       = 0; // /NART
		pdmocctl->cmd[i].txqcan.can.dlc    = 8; // All command msgs have 8 payload bytes

		for (j = 0; j < 8; j++) // Clear out payload (later, some bytes are bytes are overwritten)
		{
			pdmocctl->cmd[i].txqcan.can.cd.uc[j] = 0;
		}
	}
	
/* Load CAN id into DMOC command CAN msgs. */

	//Commanded RPM plus state of key and gear selector
	// CANID_DMOC_CMD_SPEED', '46400000','DMOC','I16_X6',         'DMOC: cmd: speed, key state'
	pdmocctl->cmd[CMD1].txqcan.can.id = gevcufunction.lc.cid_dmoc_cmd_speed;

	//Torque limits
	// CANID_DMOC_CMD_TORQ',  '46600000','DMOC','I16_I16_I16_X6', 'DMOC: cmd: torq,copy,standby,status
	pdmocctl->cmd[CMD2].txqcan.can.id = gevcufunction.lc.cid_dmoc_cmd_torq;

	//Power limits plus setting ambient temp and whether to cool power train or go into limp mode
	//CANID_DMOC_CMD_REGEN', '46800000','DMOC','I16_I16_X_U8_U8','DMOC: cmd: watt,accel,degC,alive
	pdmocctl->cmd[CMD3].txqcan.can.id = gevcufunction.lc.cid_dmoc_cmd_regen;

/* Preset some payload bytes that do not change. */
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[4] = 0x75; // msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[5] = 0x30; // lsb
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[4] = 0x75; // msb standby torque. -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[5] = 0x30; // lsb

	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[5] = 60;   // 20 degrees celsius

	return;
}
/* ***********************************************************************************************************
 * void dmoc_control_time(struct DMOCCTL* pdmocctl, uint32_t ctr);
 * @brief	: Timer input to state machine
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @param	: ctr = sw1ctr time ticks
 ************************************************************************************************************* */
void dmoc_control_time(struct DMOCCTL* pdmocctl, uint32_t ctr)
{
	if (pdmocctl->state == DMOCINIT1)
	{ // OTO 
		pdmocctl->nextctr = ctr + DMOC_KATICKS;
		pdmocctl->sendflag = 1;       // Trigger sending now
		pdmocctl->state = DMOCINIT2;  // 
		return;
	}

	/* Keepalive timing, based on sw1tim time ticks (e.g. (1/128) ms). */
	if ((int)(pdmocctl->nextctr - ctr)  > 0) return;

	/* Set next ctr for next KA msg. */
	pdmocctl->nextctr = ctr + DMOC_KATICKS; // Send next KA msg
	pdmocctl->sendflag = 1; // Send first msg

	return;
}
/* ***********************************************************************************************************
 * void dmoc_control_GEVCUBIT08(struct DMOCCTL* pdmocctl, struct CANRCVBUF* pcan);
 * @brief	: CAN msg received: cid_dmoc_actualtorq
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @param	: pcan = pointer to CAN msg struct
 ************************************************************************************************************* */
void dmoc_control_GEVCUBIT08(struct DMOCCTL* pdmocctl, struct CANRCVBUF* pcan)
{
	/* Extract reported torque and update latest reading. */
//				torqueActual = ((frame->data.bytes[0] * 256) + frame->data.bytes[1]) - 30000;
	pdmocctl->torqueact = ((pcan->cd.uc[0] << 8) + (pcan->cd.uc[1])) - pdmocctl->torqueoffset;
	return;
}

/* ***********************************************************************************************************
 * void dmoc_control_CANsend(struct DMOCCTL* pdmocctl);
 * @brief	: Send group of three CAN msgs to DMOC
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 ************************************************************************************************************* */
/*
	This is called from GevcuUpdates.c. If the 'sendflag' is set during GevcuEvents, or GevcuStates,
   the command CAN messages are sent.
*/
void dmoc_control_CANsend(struct DMOCCTL* pdmocctl)
{
	int32_t ntmp;

	if (pdmocctl->sendflag == 0) return; // Return when not flagged to send.
	pdmocctl->sendflag = 0; // Reset flag
	
	/* Increment 'alive' counter by two each time group of three is sent */
	pdmocctl->alive = (pdmocctl->alive + 2) & 0x0F;

	/* CMD1: RPM plus state of key and gear selector ***************  */
	// Update payload
	ntmp = pdmocctl->speedreq; // JIC speedreq gets changed between following instruction
	pdmocctl->cmd[CMD1].txqcan.can.cd.uc[0] = (ntmp & 0xFF00) >> 8;
	pdmocctl->cmd[CMD1].txqcan.can.cd.uc[1] = (ntmp & 0x00FF);
	pdmocctl->cmd[CMD1].txqcan.can.cd.uc[5] = 1; // Key state = ON (this could be in 'init')
	pdmocctl->cmd[CMD1].txqcan.can.cd.uc[6] = pdmocctl->alive | (DMOC_DRIVE << 4) | (pdmocctl->dmocstatereq << 6) ;
	pdmocctl->cmd[CMD1].txqcan.can.cd.uc[7] = DMOCchecksum(&pdmocctl->cmd[CMD1].txqcan.can); 

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&pdmocctl->cmd[CMD1].txqcan,4);

	/* CMD2: Torque limits ****************************************** */
	// Update payload
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[0] = (pdmocctl->speedreq & 0xFF00) >> 8;
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[1] = (pdmocctl->speedreq & 0x00FF);

	/* Speed or Torque mode. */
	if (pdmocctl->mode == DMOC_MODETORQUE)
	{ // Torque mode
/* For speed mode they seem to be controlling speed by adjusting the torque! */
      if(pdmocctl->speedact < pdmocctl->maxspeed) 
		{ //If actual rpm is less than max rpm, add torque to offset
            pdmocctl->torquereq += pdmocctl->torquereq;   
      }
      else 
		{  // Reduce torque
/* Why do they use "1/1.3" for reducing the torque to reduce speed? */
      	pdmocctl->torquereq += pdmocctl->torquereq /1.3;  
      }
		ntmp = pdmocctl->torquereq; // JIC torquereq gets changed between following instructions
		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[0] = (ntmp & 0xFF00) >> 8;
		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[2] = (ntmp & 0xFF00) >> 8;
		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[1] = (ntmp & 0x00FF);
		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[3] = (ntmp & 0x00FF);
	}
	else
	{ // Speed mode
		ntmp = pdmocctl->torquereq; // JIC torquereq gets changed between following instructions
		ntmp += pdmocctl->maxtorque;
		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[0] = (ntmp & 0xFF00) >> 8;
		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[1] = (ntmp & 0x00FF);
//      -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
//		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[2] = 0x75; //zero torque [Set during init]
//		pdmocctl->cmd[CMD2].txqcan.can.cd.uc[3] = 0x30; [Set during init]
	}
	
	//what the hell is standby torque? Does it keep the transmission spinning for automatics? I don't know.
//   -3000 offset, 0.1 scale. These bytes give a standby of 0Nm
//	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[4] = 0x75; //msb standby torque. [Set during init]
//	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[5] = 0x30; //lsb [Set during init]

	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[6] = pdmocctl->alive;
	pdmocctl->cmd[CMD2].txqcan.can.cd.uc[7] = DMOCchecksum(&pdmocctl->cmd[CMD2].txqcan.can); 

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&pdmocctl->cmd[CMD2].txqcan,4);

	/* CMD3: Power limits plus setting ambient temp *************** */
  	  // [Could these two be an OTO init, or are they updated as the battery sags?]
	pdmocctl->regencalc = 65000 - (pdmocctl->maxregenwatts / 4);
	pdmocctl->accelcalc = (pdmocctl->maxaccelwatts / 4);

	// Update payload
	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[0] = ((pdmocctl->regencalc & 0xFF00) >> 8); //msb of regen watt limit
	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[1] = (pdmocctl->regencalc & 0xFF); //lsb
	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[2] = ((pdmocctl->accelcalc & 0xFF00) >> 8); //msb of acceleration limit
	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[3] = (pdmocctl->accelcalc & 0xFF); //lsb
//	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[4] = 0; //not used [Set during init]
//	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[5] = 60; //20 degrees celsius [Set during init]
	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[6] = pdmocctl->alive;
	pdmocctl->cmd[CMD3].txqcan.can.cd.uc[7] = DMOCchecksum(&pdmocctl->cmd[CMD3].txqcan.can); 

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&pdmocctl->cmd[CMD3].txqcan,4);

	return;
}
/* ***********************************************************************************************************
 * void dmoc_control_throttlereq(struct DMOCCTL* pdmocctl, float fpct);
 * @brief	: Convert 0 - 100 percent into torquereq
 * @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @param	: fpct = throttle (control lever) position: 0.0 - 100.0
 ************************************************************************************************************* */
void dmoc_control_throttlereq(struct DMOCCTL* pdmocctl, float fpct)
{
	
}

