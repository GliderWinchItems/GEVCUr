/******************************************************************************
* File Name          : cdc_rxbuffTaskCAN.c
* Date First Issued  : 10/27/2019
* Description        : Incoming ASCII format CAN msgs from PC -> binary CAN 
*******************************************************************************/
#include <malloc.h>
#include "cdc_rxbuffTaskCAN.h"
#include "usbd_cdc_if.h"
#include "CanTask.h"
#include "SerialTaskReceive.h"

/* CDC buffers. */
struct CDCOUTBUFF cdcbuf[CDCOUTNUMBUF];
struct CDCOUTBUFF* pcdcbuf_add;
struct CDCOUTBUFF* pcdcbuf_take;

/*
struct GATEWAYPCTOCAN
{
	struct CANRCVBUFPLUS*	pcanp; // Ptr into buffer for received CAN Plus msg
	uint32_t chksumx;       // Checksum in progress
	uint8_t binseq;         // Received sequence number (binary)
	uint8_t ctrseq;			// Software maintained sequence number
	uint8_t state;          // State of decoding
	uint8_t error;          // Error code: 0 = no errors
	uint8_t bin;            // Bin byte in progress
	uint8_t odd;            // Nibble: Odd = 1, even = 0;
	uint8_t ctr;            // Data storing counter
};
struct CANTXQMSG
{
	struct CAN_CTLBLOCK* pctl;	// Pointer to control block for this CAN
	struct CANRCVBUF can;		// CAN msg
	uint8_t maxretryct;
	uint8_t bits;
};
struct CDCRXCANMSG
{
	struct CANRCVBUF can;
	uint8_t binseq;         // Received sequence number (binary)
	uint8_t error;          // Error code: 0 = no errors
};
*/
#define CDCNUMCANBUF	12	// Number of CAN msgs buffered
static struct GATEWAYPCTOCAN pctocan;
static struct CDCRXCANMSG  cdccan[CDCNUMCANBUF];
static struct CDCRXCANMSG* pcan_add;
static struct CDCRXCANMSG* pcan_take;


static osThreadId tskhandle = NULL; // Task handle for task "consuming" lines
static uint32_t notebit;

osThreadId CdcRxTaskReceiveCANHandle = NULL; // We met the task and it is us.
void StartCdcRxTaskReceiveCAN(void const * argument);

/* Lookup table to convert one hex char to binary (4 bits), no checking for illegal incoming hex */
static const uint8_t hexbin[256] = {
/*          0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15   */
/*  0  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  1  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  2  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  3  */   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
/*  4  */   0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  5  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  6  */   0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  7  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  8  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  9  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 10  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 11  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 12  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 13  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 14  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
/* 15  */   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};
/* ****************************************************************************
  * static void newcan_init(void);
  * @brief	: set initial vars for building a new CAN msg
  *******************************************************************************/
static void newcan_init(void)
{
	/* Initialize for new CAN msg construction */

	struct GATEWAYPCTOCAN* p = &pctocan;
	p->state   = 0;
	p->error   = 0;
	p->ctr     = 0;
	p->odd     = 1;
	p->chksumx = CHECKSUM_INITIAL;	// Checksum initial value

	return;
}

/** ****************************************************************************
  * static void buff_init(void);
  * @brief	: Set pointers for buffers
  *******************************************************************************/
static void buff_init(void)
{
	/* CDC buffer add and take pointers. */
	pcdcbuf_add  = &cdcbuf[0];
	pcdcbuf_take = &cdcbuf[0];

	/* CANXQ msg circular buffering pointers. */
	pcan_add  = &cdccan[0];
	pcan_take = &cdccan[0];

	return;
}
/* *************************************************************************
 * osThreadId xCdcRxTaskReceiveCANCreate(uint32_t taskpriority, uint32_t nb);
 * @brief	: Create task to serve calling task
 * @param	: taskpriority = Task priority (just as it says!)
 * @param	: nb = notification bit used with calling task
 * @return	: Handle to crearted task
 * *************************************************************************/
osThreadId xCdcRxTaskReceiveCANCreate(uint32_t taskpriority, uint32_t nb)
{
	/* Init buffer pointers. */
	buff_init();

	/* Init for first new CAN msg. */
	newcan_init();

	/* Save task handle that 'take's the CAN msgs. */
	tskhandle = xTaskGetCurrentTaskHandle();

	/* Save for notification bit pattern. */
	notebit = nb;

	/* definition and creation of task: CdcTxTaskSend */
   osThreadDef(CdcRxTaskReceiveCAN, StartCdcRxTaskReceiveCAN, osPriorityNormal, 0,128);
   CdcRxTaskReceiveCANHandle = osThreadCreate(osThread(CdcRxTaskReceiveCAN), NULL);
	vTaskPrioritySet( CdcRxTaskReceiveCANHandle, taskpriority );

	return CdcRxTaskReceiveCANHandle;
}
/* *************************************************************************
 * void StartCdcRxTaskReceiveCAN(void const * argument);
 *	@brief	: Task startup
 * @return	: struct CDCRXCANMSG->error holds results for each CAN msg     
 *				:      0  = no errors
 *				: (1<<0) |=  1 not assigned
 *          : (1<<1) |=  2 completed, but bad checksum
 *  		   : (1<<2) |=  4 line terminator and state sequence not complete
 *		      : (1<<3) |=  8 sequence number did not mismatch
 *		      : (1<<4) |= 16 too many chars
 *          : (1<<5) |= 32 DLC greater than 8 (too large)
 * *************************************************************************/
// Debug vars
uint32_t dbcdcrx;
uint32_t	dbrxbuff; // Number of CDC buffer lengths larger than 64.
uint32_t dblen;

void StartCdcRxTaskReceiveCAN(void const * argument)
{
	uint8_t* pcdc;
	uint8_t c;

	struct GATEWAYPCTOCAN* p = &pctocan;

  /* Infinite loop */
  for(;;)
  {
		/* Wait for usbd_cdc_if.c CDC_Receive_FS to signal input received. */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		/* Unload all CDC buffers, and convert to CAN msgs */
		while (pcdcbuf_add != pcdcbuf_take)
		{ // Here, one or more CDC buffers to unload.
dblen = pcdcbuf_take->len; // Save for debugging

			/* Build CAN msgs until newline encountered. */
			if (pcdcbuf_take->len != 0)
			{ // Here, we have CDC data copy to line buffer(s). */
if (pcdcbuf_take->len <= CDCOUTBUFFSIZE) // Bogus length?
{ // No, not bogus and therefore not a trouble-maker.
				pcdc = &pcdcbuf_take->u8[0]; // Byte pointer into CDC buffer
				while (pcdcbuf_take->len > 0)
				{	
					pcdcbuf_take->len -= 1;
					c = *pcdc++; // Get CDC byte

					/* CAN msg ascii/hex separated with LINETERMINATOR. */
					// 0x0D takes care of someone typing in stuff with minicom
					if ((c == 0XD) || (c == LINETERMINATOR))
					{ // Here End of Line
						
						/* End of line signals end of CAN msg; beginning of new.  */
						if (p->state != 7) // Did it end correctly?
						{ // Here, no.
							p->error |= (1<<2);	// Line terminator came at wrong place.
						}

						/* Give the user of the CAN msg some info. */
//						pcan_add->seq   = p->binseq;
//						pcan_add->error = p->error;

						/* Step to next CANXQ buffer. */
						pcan_add += 1;
						if (pcan_add >= &cdccan[CDCOUTNUMBUF])
								pcan_add = &cdccan[0];

dbrxbuff += 1;

						/* Notify originating task that a CAN msg is ready. */
						xTaskNotify(tskhandle, notebit,eSetBits);

						/* Initialize for next CAN msg */
						newcan_init();
					}
					else
					{ // Not end-of-line.  Convert ascii/hex to binary bytes
						if (p->odd == 1)
						{ // High order nibble
							p->odd = 0;
							p->bin = hexbin[(uint8_t)c] << 4; // Lookup binary, given ascii
						}
						else
						{ // Low order nibble completes byte
							p->odd = 1;
							p->bin |= hexbin[(uint8_t)c];
				
							/* Store binary bytes directly into CAN buffer location */
							/* Build checksum as we go. */
							switch(p->state)
							{
							case 0: // Sequence number
								p->binseq   = p->bin;
								p->chksumx += p->bin;
								p->state   += 1;
								break;
							case 1: // Low order byte of CAN id word
								pcan_add->can.id   = (p->bin << 0);
								p->state   += 1;
								p->chksumx += p->bin;
								break;
							case 2:
								pcan_add->can.id  |= (p->bin << 8);
								p->chksumx += p->bin;
								p->state   += 1;
								break;
							case 3:
								pcan_add->can.id  |= (p->bin << 16);
								p->chksumx += p->bin;
								p->state   += 1;
								break;
							case 4: // High order byte of CAN id word
								pcan_add->can.id  |= (p->bin << 24);
								p->chksumx += p->bin;
								p->state   += 1;
								break;
							case 5: // DLC byte -> CAN msg dlc word
								if (p->bin > 8)
								{ // DLC too large.
									p->bin = 8; //Do not overrun array!
									p->error |= (1<<5);
								}
								pcan_add->can.dlc = p->bin;
								p->chksumx   += p->bin;
								p->state     += 1;
								break;
							case 6: // Fill data bytes per dlc
								if (p->ctr < pcan_add->can.dlc)
								{
									pcan_add->can.cd.uc[p->ctr] = p->bin;
									p->chksumx += p->bin;
									p->ctr     += 1;
									break;
								}
								/* Here, checksum check.  Complete checksum calculation. */
								p->chksumx += (p->chksumx >> 16); // Add carries into high half word
								p->chksumx += (p->chksumx >> 16); // Add carry if previous add generated a carry
								p->chksumx += (p->chksumx >> 8);  // Add high byte of low half word
								p->chksumx += (p->chksumx >> 8);  // Add carry if previous add generated a carry
								if ((p->chksumx & 0xff) != p->bin)
								{ // Here, checksums mismatch
									p->error |=  (1<<1);
								}
			
								/* Check for missing msgs. */
								p->ctrseq += 1;	// Advance software maintained sequence number
								if (p->binseq != p->ctrseq)
								{
									p->error  |= (1<<3);	// Sequence number mismatch
									p->ctrseq = p->binseq; // Reset
								}
								p->state = 7;
								break;
			
							case 7:
								p->error |= (1<<4); // Too many chars
								break;
							} // End: switch(p->state)	
						} // End: Low order nibble completes byte
					} // End: if ((c == 0XD) || (c == LINETERMINATOR))
				} // End: while (pcdcbuf_take->len > 0)
}
else
{ // if (pcdcbuf_take->len <= CDCOUTBUFFSIZE) // Bogus length?
	
}
			} // End: if (pcdcbuf_take->len != 0)

			/* Step to next CDC buffer. */
			pcdcbuf_take += 1;
			if (pcdcbuf_take >= &cdcbuf[CDCOUTNUMBUF]) 
				 pcdcbuf_take  = &cdcbuf[0];			

		} // End: while (pcdcbuf_add != pcdcbuf_take)
  } // End: for (;;)
while(1==1);
  return; // Never get here
}
/* *************************************************************************
 * struct CDCRXCANMSG* cdc_rxbuffCAN_getCAN(void);
 *	@brief	: Get pointer to CAN msg (w error and sequence)
 * @return	: NULL = no new data, otherwise pointer to struct
 * *************************************************************************/
struct CDCRXCANMSG* cdc_rxbuffCAN_getCAN(void)
{
	struct CDCRXCANMSG* ptmp;

	/* Any new data? */
	if (pcan_add == pcan_take) return NULL;

	ptmp = pcan_take;

	/* Advance 'take' pointer to next CAN msg buffer */
	pcan_take += 1;
	if (pcan_take >= &cdccan[CDCNUMCANBUF])
		  pcan_take = &cdccan[0];

	return ptmp;
}

