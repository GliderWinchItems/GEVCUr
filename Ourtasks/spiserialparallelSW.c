/******************************************************************************
* File Name          : spiserialparallelSW.c
* Date First Issued  : 10/13/2019
* Description        : SPI serial<->parallel extension with switch debounce
*******************************************************************************/

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "spiserialparallelSW.h"
#include "malloc.h"
#include "morse.h"

static uint16_t spinotifyctr; // Count interrupts, reset to zero

/**
  * @brief  Transmit and Receive an amount of data in non-blocking mode with Interrupt.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pTxData pointer to transmission data buffer
  * @param  pRxData pointer to reception data buffer
  * @param  Size amount of data to be sent and received
  * @retval HAL status
  */
//HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)

/* Input and Output mirror of hw registers. */
union SPISP spisp_rd[SPISERIALPARALLELSIZE/2];
union SPISP spisp_wr[SPISERIALPARALLELSIZE/2];

/* Previous 'read' word, used to check for switch changes. */
static union SPISP spisp_rd_prev[SPISERIALPARALLELSIZE/2];

uint32_t spispctr; // spi interrupt counter for debugging.

static SPI_HandleTypeDef *pspix;

static uint16_t spinotebits; // Shift0reg bits that changed
static int16_t srinit;	// Initialization cycle ctr


/* NOTE: Switch i/o pins have pull-ups and closing the switch pulls the
   pin to ground, hence, a zero bit represents a closed contact. */

enum SWPAIRSTATE
{
	SWP_UNDEFINED, /* 0 Sw state is initially undefined */
	SWP_NC,        /* 1 Normally closed contact */
	SWP_NO,        /* 2 Normally opened contact */
	SWP_DEBOUNCE,  /* 3 NC has made contact, but debounce in progress */
	SWP_DEBOUNCE_OPENING,
	SWP_DEBOUNCE_CLOSING,
};

/* Pushbutton states. */
enum SWPBSTATE
{
	SWPB_OPEN,
	SWPB_CLOSED,
	SWPB_OPENING,
	SWPB_CLOSING
};

/* Reconstructed Local copy (possibly delayed slightly) of spi read word. */
uint16_t spilocal;

uint32_t spitickctr; // Running count of spi time ticks (mostly for debugging)

// TODO make these static after debugging
/* Pushbutton linked list head pointer. */
struct SWITCHPTR* phdsw = NULL; // Pointer link head: switches instantiated
struct SWITCHPTR* phddb = NULL; // Pointer link head: active debouncing

/* Array for struct access via switch bit position. */
struct SWITCHPTR* pchange[NUMSWS]  = {0};

/* *************************************************************************
 * static void debouncing_add(struct SWITCHPTR* p);
 *	@brief	: Add 'this' switch to the list that is active debouncing.
 * @param	: p = pointer to struct for this pushbutton 
 * *************************************************************************/
static void debouncing_add(struct SWITCHPTR* p)
{
	if (phddb == NULL)
	{ // Here, no switches are active debouncing 
		phddb = p;       // Point debounce head to this switch struct
		p->pdbnx = NULL; // This switch is the last on the debouncing list
	}
	else
	{ // Here, one or more on debouncing list
		// Add to head of list
		p->pdbnx = phddb;
		phddb = p;
	}
	return;
}
/*******************************************************************************
Inline assembly.  Hacked from code at the following link--
http://balau82.wordpress.com/2011/05/17/inline-assembly-instructions-in-gcc/
*******************************************************************************/
static inline __attribute__((always_inline)) uint32_t arm_clz(uint32_t v) 
{
  unsigned int d;
  asm ("CLZ %[Rd], %[Rm]" : [Rd] "=r" (d) : [Rm] "r" (v));
  return d;
}

/* *************************************************************************
 * struct SWITCHPTR* switch_pb_add(osThreadId tskhandle, uint32_t notebit, 
    uint32_t switchbit,
    uint32_t switchbit1,
    uint8_t type,
    uint8_t db_mode, 
    uint8_t dur_closing,
    uint8_t dur_opening);
 *
 *	@brief	: Add a single on/off (e.g. pushbutton) switch on a linked list
 * @param	: tskhandle = Task handle; NULL to use current task; 
 * @param	: notebit = notification bit; 0 = no notification
 * @param	: switchbit  = bit position in spi read word for this switch
 * @param	: switchbit1 = bit position for 2nd switch if a switch pair
 * @param	: type: 0 = on/off (pushbutton), 1 = switch pair
 * @param	: db_mode = debounce mode: 0 = immediate; 1 = wait debounce
 * @param	: dur_closing = number of spi time tick counts for debounce
 * @param	: dur_opening = number of spi time tick counts for debounce
 * @return	: Pointer to struct for this switch
 * *************************************************************************/
 struct SWITCHPTR* switch_pb_add(osThreadId tskhandle, uint32_t notebit, 
    uint32_t switchbit,
    uint32_t switchbit1,
    uint8_t type,
    uint8_t db_mode, 
    uint8_t dur_closing,
    uint8_t dur_opening)
{
	struct SWITCHPTR* p1 = phdsw;
	struct SWITCHPTR* p2;
	uint16_t n,nn;

taskENTER_CRITICAL();

	if (p1 == NULL)
	{ // Here, get first sw struct for list
		p2 = (struct SWITCHPTR*)calloc(1, sizeof(struct SWITCHPTR));
		if (p2 == NULL){taskEXIT_CRITICAL(); morse_trap(524);}
		phdsw = p2; // Head now points to 1st on list.
	}
	else
	{ // One or more on linked list

// TODO Error-check if switch already on linked list

		while (p1->pnext != NULL) // Find last item
		{
			p1 = p1->pnext;
		}
		p2 = (struct SWITCHPTR*)calloc(1, sizeof(struct SWITCHPTR));
		if (p2 == NULL){taskEXIT_CRITICAL(); morse_trap(525);}

		// Point previous last item to newly added item.
		p1->pnext = p2; 
	}

	/* Initialize linked list switch struct */
// Redundant: calloc takes care of this
	p2->pdbnx = NULL; // Not active debouncing

	// Task Notification. notebit = 0 means skip notification.
	if (tskhandle == NULL) // Use calling task handle?
		 tskhandle  = xTaskGetCurrentTaskHandle();
	p2->tskhandle  = tskhandle;
	p2->notebit    = notebit;   // Notification bit to use

	// Designation of the switch(s) in the spi word
	p2->switchbit  = switchbit; // 1st sw position in spi word 
	p2->switchbit1 = switchbit1;// 2nd sw if a switch pair

	// Debouncing logic & timing
// Note: calloc sets to zero, some of the following is redundant
	p2->on         = 0;      // Initial sw contact status (closed)   
	p2->db_on      = 0;      // Initial post-debounce status (closed)
	p2->db_ctr     = 0;      // Working debounce timing counter
	p2->type       = type;   // Switch type: pushbutton
	p2->state      = SWPB_CLOSED; // vars initially zero, so closed
	p2->db_dur_closing = dur_closing; // spi tick count closing
	p2->db_dur_opening = dur_opening; // spi tick count opening
	p2->db_mode        = db_mode;     // Type of logic applied

	/* Map switch bit position (of xor'd changes) to this new struct. */
	// This allows bit position to find sw struct without search
	n = arm_clz(p2->switchbit); // Count leading zeros
	if (n == 32) morse_trap(526); // Prog error: switchbit was zero!
	nn = 31 - n; // n=31 is bit position 0; n=0 is bit pos 31
	// Currently, only two bytes in the spi word.
	if (nn > (SPISERIALPARALLELSIZE*8)) morse_trap(527); // Bit pos out-of-range
	pchange[nn] = p2; // Bit position pointer to new struct for 1st switch

	/* If this is a switch pair set point in array. */
	if (p2->switchbit1 != 0) // Was a 2nd switch identified?
	{ // Here, yes, this is a switch pair
		n = arm_clz(p2->switchbit1); // Count leading zeros
		nn = 31 - n; // n=31 is bit position 0; n=0 is bit pos 31
		if (nn > (SPISERIALPARALLELSIZE*8)) morse_trap(527); // Bit pos out-of-range
		pchange[nn] = p2; // Bit position pointer for 2nd switch
	}
	
taskEXIT_CRITICAL();
	return p2; // Return pointer to first switch (in case of a pair)
}

/* *************************************************************************
 * static void dbends(struct SWITCHPTR* p);
 *	@brief	: Handle debounce duration ended.
 * @param	: p = pointer to struct for a sw on the debounce linked list
 * *************************************************************************/
static void dbends(struct SWITCHPTR* p)
{
	BaseType_t xHPT = pdFALSE;
	/* Here, the end of the debounce duration. */
	switch(p->state)
	{
	case SWPB_OPENING: // Switch is in process of opening
		if (p->on == SW_CLOSED)
		{ // Here, sw closed during a sw opening debounce period
			p->state = SWPB_CLOSED;
			if (p->db_mode == SWMODE_NOW)
			{
				p->db_on = SW_CLOSED; // Show representation sw closed
				if ((p->notebit != 0) && (p->tskhandle != NULL))

					xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);
			}
		}
		else
		{ // Here, sw is open and state is opening (from closed)
			p->state = SWPB_OPEN; // Set constant open state
			if (p->db_mode == SWMODE_WAIT)
			{ // Here, mode is wait until debounce completes
				p->db_on = SW_OPEN;   // Show representation sw open
				if ((p->notebit != 0) && (p->tskhandle != NULL))
					xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);
			}
		}
		break;		

	case SWPB_CLOSING: // Switch is in process of closing
		if (p->on != SW_CLOSED)			
		{ // Here, sw opened during the debounce duration
			p->state = SWPB_OPEN; // We are back to open state
			if (p->db_mode == SWMODE_NOW)
			{
				p->db_on = SW_OPEN;   // Show representation sw open
				if ((p->notebit != 0) && (p->tskhandle != NULL))
					xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);
			}
	//    else{ // mode is delay/wait, so db_on remains unchanged}
		}
		else
		{ // Here, debounce complete and sw still closed
			p->state = SWPB_CLOSED;				
			if (p->db_mode == SWMODE_WAIT)
			{
				p->db_on = SW_CLOSED;
				if ((p->notebit != 0) && (p->tskhandle != NULL))
					xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);
			}
		}
		break;
	}
	return;
}
/* *************************************************************************
 * static void pbxor(struct SWITCHPTR* p);
 *	@brief	: logic for pushbutton bit *changed*
 * @param	: p = pointer to struct for this pushbutton 
 * @param	: mask for clearing bits
 * *************************************************************************/
static void pbxor(struct SWITCHPTR* p)
{
	BaseType_t xHPT = pdFALSE;

	/* Save present contact state: 1 = open, 0 = closed. */
	p->on = p->switchbit & spisp_rd[0].u16; 

	switch(p->state)
	{
	case SWPB_OPEN: // Current debounce state: open
		if (p->on == SW_CLOSED)
		{ // Here, change: open state -> closed contact
			if (p->db_dur_closing == 0) // Debounce count?
			{ // Special case: instantaneous state change, no debounce
				p->db_on = SW_CLOSED;    // Debounced: representative of sw
				p->state = SWPB_CLOSED;  // New sw state
				if ((p->notebit != 0) && (p->tskhandle != NULL))
				{
					xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);	
					portYIELD_FROM_ISR( xHPT ); // Trigger scheduler
				}
			}
			else
			{ // One or more debounce ticks required.
				if (p->db_mode == SWMODE_NOW)
				{ // Debounced sw representation shows new contact state "now"
					p->db_on = SW_CLOSED;
					if ((p->notebit != 0) && (p->tskhandle != NULL)) // Notify new state, if indicated
					{
						xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);	
						portYIELD_FROM_ISR( xHPT ); // Trigger scheduler
					}
				}
				p->state = SWPB_CLOSING; // New state: closing is in process
				p->db_ctr = p->db_dur_closing; // Debounce count
				debouncing_add(p); // Add to debounce active linked list
			}
		}
		else
		{ // Here, change: open state -> open contact (what!?)
			p->db_on = SW_OPEN;
		}
		break;

	case SWPB_CLOSED: // Current debounce state: closed
		if (p->on != SW_CLOSED) // Not closed?
		{ // Here, change: closed state -> open contact
			if (p->db_dur_opening == 0) // Debounce count?
			{ // Special case: instantaneous state change
				p->db_on = SW_OPEN;
				p->state = SWPB_OPEN;
				if ((p->notebit != 0) && (p->tskhandle != NULL))
				{
					xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);	
					portYIELD_FROM_ISR( xHPT ); // Trigger scheduler
				}
			}
			else
			{ // One or more debounce ticks required.
				if (p->db_mode == SWMODE_NOW)
				{
					p->db_on = SW_OPEN;
					if ((p->notebit != 0) && (p->tskhandle != NULL))
					{
						xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);	
						portYIELD_FROM_ISR( xHPT ); // Trigger scheduler
					}
				}
				p->state = SWPB_OPENING;
				p->db_ctr = p->db_dur_opening; // Set debounce time counter
				debouncing_add(p); // Add to linked list
			}
		}
		else
		{ // Here, change: open state -> open contact (what!?)
			p->db_on = SW_OPEN;
		}
		break;

	case SWPB_OPENING: // sw changed during debounce period
		p->db_ctr = p->db_dur_opening; // Extend debounce time counter
		break;

	case SWPB_CLOSING: // sw changed during debounce period
		p->db_ctr = p->db_dur_closing; // Extend debounce count
		break;			

	default:
		morse_trap(544); // Programming error
	}
	return;
}
/* *************************************************************************
 * static void p2xor(struct SWITCHPTR* p);
 *	@brief	: logic for two switch pair
 * @param	: p = pointer to struct for this switch struct
 * @param	: p->db_on set with sw pair of 0b10 or 0b01.
 * *************************************************************************/
static void p2xor(struct SWITCHPTR* p)
{
	BaseType_t xHPT = pdFALSE;

	/* Convert sw pair status to a two bit value. */
	p->on = 0x0; // Both sws closed
	if ((p->switchbit & spisp_rd[0].u16) != SW_CLOSED)
		p->on |= 0x1; // 1st sw open
	if ((p->switchbit1 & spisp_rd[0].u16) != SW_CLOSED)
		p->on |= 0x2; // 2nd sw open

	switch (p->on)
	{
	case 0: // Both closed: indeterminant, no change
		break;	
	case 1: // 1st open: 2nd closed
		if (p->db_on == p->on) break;
		p->db_on = p->on;
			if ((p->notebit != 0) && (p->tskhandle != NULL)) // Notify task?
			{
				xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);	
				portYIELD_FROM_ISR( xHPT ); // Trigger scheduler
			}
		break;	
	case 2: // 1st closed: 2nd open
		if (p->db_on == p->on) break;
		p->db_on = p->on;
			if ((p->notebit != 0) && (p->tskhandle != NULL)) // Notify task?
			{
				xTaskNotifyFromISR(p->tskhandle, p->notebit, eSetBits, &xHPT);	
				portYIELD_FROM_ISR( xHPT ); // Trigger scheduler
			}
		break;	
	case 3: // Both open: indeterminant, no change
		break;	
	}
	return;
}

/* *************************************************************************
 * HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi);
 *	@brief	: Init pins & start SPI running
 * @return	: success = HAL_OK
 * *************************************************************************/
static volatile int dly;
HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi)
{
	pspix = phspi;	// Save pointer to spi contol block

	/* 790-800 is the threshold for error vs ok. */
	srinit = 3000;	// Allow a few cycles for the hardware shift-regs to init.

	/* Enable output shift register pins. */
	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_RESET); // Not OE pins
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);

	/* Start SPI running. */
	return HAL_SPI_TransmitReceive_IT(phspi, 
		&spisp_wr[0].u8[0], 
		&spisp_rd[0].u8[0], 
		SPISERIALPARALLELSIZE);
}
uint16_t sr1;
uint16_t srdiff1;
/* *************************************************************************
 * void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
 *	@brief	: HAL spin interrupt callback processing.
 * *************************************************************************/
/**
  * @brief Tx and Rx Transfer completed callback.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
// __weak void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	struct SWITCHPTR* p;
	struct SWITCHPTR* p2;

	uint32_t n,nn;            // Leading zeros, counts
	uint32_t xortmp;

	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);

/* NOTE: A delay is needed between the set and reset of the 
   I/O pin. Otherwise it is too fast for the pins to follow.
	With the elimination SwitchTask, and the code in that task
   implemented here, there is plenty of time delay.
*/

	/* Allow a couple of cycles for the hardware shift-regs. */
	if (srinit > 0)
	{ // Here, do another spi cycle before "real work".
  		srinit -= 1;
sr1 = spisp_rd[0].u16;
srdiff1 = spisp_rd_prev[0].u16;
		goto startnextcycle; // ## WHAT?! ##(old FORTRAN programmer at work)
	}

	spispctr += 1; // Debugging ctr (useful for checking rate)

	/* Set the bits that changed in the notification word. */
	spinotebits = (spisp_rd_prev[0].u16 ^ spisp_rd[0].u16);
	spisp_rd_prev[0].u16 = spisp_rd[0].u16; // Update previous

	/* ===== switch contact change. ========================= */
	if (spinotebits != 0)
	{ // Input (read) word has changed
		xortmp = spinotebits; // Working word of change bits

spilocal = spinotebits; // Save for Debugging

		/* Deal with switches that have a change. */
		while (xortmp != 0)
		{
			/* Get bit position of changed bit w asm instruction. */
			n = arm_clz(xortmp); // Count leading zeros
			if (n == 32) morse_trap(528); // Shouldn't happen

			nn = (31 - n); // n=31 is bit position 0; n=0 is bit pos 31
			if (nn >= (SPISERIALPARALLELSIZE*8)) morse_trap(529); 

			/* Point to struct for this switch */
			p = pchange[nn]; // Point to struct for this sw or sw pair

			/* If p == NULL, then it would be a non-instantiated sw, 
	         which with pullups are ones, but initial vars zero, so
	         we end up here with a NULL ptr. */
			if (p != NULL)
			{ // 
				if (p->type == SWTYPE_PB)
				{ // Here, a pushbutton type
					pbxor(p);
					xortmp = (xortmp & ~(p->switchbit) );
				}
				else
				{ // Here, not a pushbutton type
					if (p->type == SWTYPE_PAIR)
					{ // Here, a switch pair
						p2xor(p);
						/* If by chance both sw bits show changed in the spi word
						   then, p2xor will deal with both correctly, and thus
						   both bits are cleared from the spixor word. */
						xortmp = (xortmp & ~(p->switchbit|p->switchbit1) );
					}
					else
					{ // Program error (morse assert?)
						morse_trap(534);
					}
				}
			}
			else
			{ // Clear bit for non-instantiated sw
				xortmp = (xortmp & ~(1 << nn) );
			}
		}
	}
	/* ===== Timing for debouncing. ============================== */
	// Count spi interrupt to yield a reasonable time rate for deboucing
	spinotifyctr += 1; // SwitchTask update timer ctr
	if (spinotifyctr >= SPINCNOTIFYCTR)
	{ // Here, we have gone approximately 50ms
		spinotifyctr  = 0; // Reset time tick counter
		/* Traverse linked list of pushbuttons actively debouncing. */
		p = phddb;
		p2=p;
		while (p != NULL) // End of list?
		{ /* Countdown debounce timing. */
			if (p->db_ctr != 0) 
			{ // Here still counting down 
				p->db_ctr -= 1;
				if (p->db_ctr == 0) 
				{ // Here, debounced ended with this tick
					dbends(p); // Do debounce logic
		
					/* Remove from debounce list */
					if (p->pdbnx == NULL)
					{ // Here, 'p' is last on list
						if (p == phddb)
						{ // Here, it was the only one on the list
							phddb = NULL; // Set head: list empty
						}
						else
						{ // Previous list item is now last
							p2->pdbnx = NULL; 
						}
					}
					else
					{ // Here, 'p' is not last
						if (p == phddb)
						{ // Here, it is the first 
							phddb = p->pdbnx; // Head points where 1st pointed to
						}
						else
						{ // Here, somewhere in the middle
							p2->pdbnx = p->pdbnx; // Set new next for previous
						}
					}
				}
			}
			p2 = p; // Save "previous"
			p = p->pnext; // Get next in list
		}
	}
	/* ========== Start next spi cycle ==================== */
startnextcycle:
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);

	/* Re-trigger another xmit/rcv cycle. */
	HAL_SPI_TransmitReceive_IT(pspix, 
		&spisp_wr[0].u8[0], 
		&spisp_rd[0].u8[0], 
		SPISERIALPARALLELSIZE);

	return;
}
