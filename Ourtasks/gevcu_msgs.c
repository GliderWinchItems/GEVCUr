/******************************************************************************
* File Name          : gevcu_msgs.c
* Date First Issued  : 07/05/2019
* Description        : Setup and send non-command function CAN msgs
*******************************************************************************/
/*
SENT by gevcu function:
 (1) gevcu command "cid_keepalive_r" (response to "cid_keepalive_i")
     payload[0]
       bit 7 - faulted (code in payload[2])
       bit 6 - warning: minimum pre-chg immediate connect.
              (warning bit only resets with power cycle)
		 bit[0]-[3]: Current main state code

     payload[1] = critical error state error code
         0 = No fault
         1 = battery string voltage (hv1) too low
         2 = gevcu 1 de-energized, aux1 closed
         3 = gevcu 2 de-energized, aux2 closed
         4 = gevcu #1 energized, after closure delay aux1 open
         5 = gevcu #2 energized, after closure delay aux2 open
         6 = gevcu #1 does not appear closed
         7 = Timeout before pre-charge voltage reached cutoff
         8 = Contactor #1 closed but voltage across it too big
         9 = Contactor #2 closed but voltage across it too big

		payload[2]
         bit[0]-[3] - current substate CONNECTING code
         bit[4]-[7] - current substate (spare) code

 poll  (response to "cid_gps_sync") & heartbeat
 (2)  "cid_msg1" hv #1 : current #1  battery string voltage:current
 (3)	"cid_msg2" hv #2 : hv #3       DMOC+:DMOC- voltages

 function command "cid_cmd_r"(response to "cid_cmd_i")
 (4)  conditional on payload[0], for example(!)--
      - ADC ct for calibration purposes hv1
      - ADC ct for calibration purposes hv2
      - ADC ct for calibration purposes hv3
      - ADC ct for calibration purposes current1
      - ADC ct for calibration purposes current2
      - Duration: (Energize coil 1 - aux 1)
      - Duration: (Energize coil 2 - aux 2)
      - Duration: (Drop coil 1 - aux 1)
      - Duration: (Drop coil 2 - aux 2)
      - volts: 12v CAN supply
      - volts: 5v regulated supply
      ... (many and sundry)

 heartbeat (sent in absence of keep-alive msgs)
 (5)  "cid_hb1" Same as (2) above
 (6)  "cid_hb2" Same as (3) above
*/

#include "gevcu_msgs.h"
#include "MailboxTask.h"

static void hvpayload(struct GEVCUFUNCTION* pcf, uint8_t idx1,uint8_t idx2,uint8_t idx3);
static void load4(uint8_t *po, uint32_t n);

/* *************************************************************************
 * void gevcu_msg1(struct GEVCUFUNCTION* pcf, uint8_t w);
 *	@brief	: Setup and send responses: battery string voltage & battery current
 * @param	: pcf = Pointer to working struct for Contactor function
 * @param	: w = switch for CID_HB1 (0) or CID_MSG1 CAN ids (1)
 * *************************************************************************/
uint32_t dbgmsg1ctr;

void gevcu_msg1(struct GEVCUFUNCTION* pcf, uint8_t w)
{
	// For loading float into payload
	union UIF
	{
		uint32_t ui;
		float    f;
	}tmp;

	uint8_t idx2;

	/* Use heartbeat or polled msg CAN id */
	if (w == 0) 
		idx2 = CID_HB1;
	else
		idx2 = CID_MSG1;

	// Load Battery string voltage (IDXHV1) as first float in payload
	hvpayload(pcf, IDXHV1, idx2, 0);

	// Battery string current as second float in payload
	double dI = (pcf->padc->cur1.iI * pcf->padc->cur1.dscale) / (1<<ADCSCALEbits);
	tmp.f = dI; // Convert to float
	load4(&pcf->canmsg[idx2].can.cd.uc[4],tmp.ui); // Load float

	pcf->canmsg[idx2].can.dlc = 8;

dbgmsg1ctr += 1;

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&pcf->canmsg[idx2],portMAX_DELAY);
	return;

}
/* *************************************************************************
 * void gevcu_msg2(struct GEVCUFUNCTION* pcf, uint8_t w);
 *	@brief	: Setup and send responses: voltages: DMOC+, DMOC-
 * @param	: pcf = Pointer to working struct for Contactor function
 * @param	: w = switch for CID_HB1 (0) or CID_MSG1 CAN ids (1)
 * *************************************************************************/
void gevcu_msg2(struct GEVCUFUNCTION* pcf, uint8_t w)
{
	uint8_t idx2;
dbgmsg1ctr += 1;

	if (w == 0) 
		idx2 = CID_HB2;
	else
		idx2 = CID_MSG2;

	// Load high voltage 2 as a float into payload
	hvpayload(pcf, IDXHV2, idx2, 0);

	// Load high voltage 3 as a float into payload
	hvpayload(pcf, IDXHV3, idx2, 4);

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&pcf->canmsg[idx2],portMAX_DELAY);
	return;
}
/* *************************************************************************
 * static void hvpayload(struct GEVCUFUNCTION* pcf, uint8_t idx1,uint8_t idx2,uint8_t idx3);
 *	@brief	: Setup and send responses: voltages: DMOC+, DMOC-
 * @param	: pcf = Pointer to working struct for Contactor function
 * @param	: idx1 = index into hv array (0-(NUMHV-1))
 * @param	: idx2 = index into ncan msg (0-(NUMCANMSGS-1))
 * @param	: idx3 = index into payload byte array
 * *************************************************************************/
static void hvpayload(struct GEVCUFUNCTION* pcf, uint8_t idx1,uint8_t idx2,uint8_t idx3)
{
	union UIF
	{
		uint32_t ui;
		float    f;
	}tmp;

	// Load high voltage [idx1] as a float into payload msg [idx2] payload byte [idx3]
	pcf->hv[idx1].dhvc = (double)pcf->hv[idx1].dscale * (double)pcf->hv[idx1].hv;
	tmp.f = pcf->hv[idx1].dhvc;                       // Convert to float
	load4(&pcf->canmsg[idx2].can.cd.uc[idx3],tmp.ui); // Load payload
	return;
}
/* *************************************************************************
 * void gevcu_msg_ka(struct GEVCUFUNCTION* pcf);
 *	@brief	: Setup and send Keep-alive response
 * @param	: pcf = Pointer to working struct for Contactor function
 * *************************************************************************/
uint32_t dbgkactr;
void gevcu_msg_ka(struct GEVCUFUNCTION* pcf)
{
dbgkactr += 1;
	/* Return command byte w primary state code */
	pcf->canmsg[CID_KA_R].can.cd.uc[0]  = 
     (pcf->pmbx_cid_keepalive_i->ncan.can.cd.uc[0] & 0xf0) |
     (pcf->state & 0xf);

	/* Fault code */
	pcf->canmsg[CID_KA_R].can.cd.uc[1] = pcf->faultcode;

	/* substate codes */
	pcf->canmsg[CID_KA_R].can.cd.uc[2]  = (pcf->substateC << 0) & 0xf;
	pcf->canmsg[CID_KA_R].can.cd.uc[2] |= (pcf->substateX << 4);

	pcf->canmsg[CID_KA_R].can.dlc = 3; // Payload size

	// Queue CAN msg
	xQueueSendToBack(CanTxQHandle,&pcf->canmsg[CID_KA_R],portMAX_DELAY);
	return;
}
/* *************************************************************************
 * static void load4(uint8_t *po, uint32_t n);
 *	@brief	: Copy uint32_t into byte array (not aligned)
 * *************************************************************************/
static void load4(uint8_t *po, uint32_t n)
{
	*(po + 0) = (n >>  0);
	*(po + 1) = (n >>  8);
	*(po + 2) = (n >> 16);
	*(po + 3) = (n >> 24);
	return;
}
