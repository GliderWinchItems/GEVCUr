/******************************************************************************
* File Name          : SwitchTask.c
* Date First Issued  : 02/042020
* Description        : Updating switches from spi shift register
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"

#include "SwitchTask.h"
#include "spiserialparallel.h"
#include "SpiOutTask.h"
#include "shiftregbits.h"
#include "GevcuTask.h"
#include "morse.h"

/* Theoretical: 10 spi cycles per millisecond, and each has a switch change
   that puts a uint16_t word on the queue. Obviously, something wrong with
   shift-register and switches. 128 words would be about 12.8 ms before there
   would be a queue overrun. */
#define SWITCHQSIZE 128

osThreadId SwitchTaskHandle    = NULL;
osMessageQId SwitchTaskQHandle = NULL;


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

/*
#define SW_SAFE     (1 <<  7)	//	F7 77  P8-3  IN 0 H
#define SW_ACTIVE   (1 <<  6)	//	F7 B7  P8-4  IN 1 G
#define PB_ARM      (1 <<  5)	//	77 57  P8-5  IN 2 F
#define PB_PREP     (1 <<  4)	//	77 67  P8-6  IN 3 E
#define CL_RST_N0   (1 <<  3)	//	7F 77  P8-7  IN 4 D
#define CP_ZTENSION (1 <<  2) // 77 73  P8-8  IN 5 C
#define CP_ZODOMTR  (1 <<  1) // 77 75  P8-9  IN 6 B
#define CL_FS_NO    (1 <<  0)	// 7F 7E  P8_10 IN 7 A

// High byte not yet connected (?)
#define CP_SPARE1  (1 << 15)  // Not on connector   IN 15 A
#define CP_SPARE2  (1 << 14)  // Not on connector   IN 14 B
#define CP_DUNNO1  (1 << 13)  // Zero tension P8-16 IN 13 C
#define CP_DUNNO2  (1 << 12)  // Zero odometerP8-15 IN 12 D
#define CP_JOGRITE (1 << 11)  // Joggle Right P8-14 IN 11 E *
#define CP_JOGLEFT (1 << 10)  // Joggle Left  P8-13 IN 10 F *
#define CP_GUILLO  (1 <<  9)  // Guillotine   P8-12 IN  9 G
#define CP_BRAKE   (1 <<  8)  // Brake        P8-11 IN  8 H GevcuTask.h
*/

/* Logical result of SW_SAFE and SW_ACTIVE */
uint8_t sw_active;  // 1 = active; 0 = safe
struct SWITCHPTR swpair_safeactive;

/* Reconstructed Local copy (possibly delayed slightly) of spi read word. */
uint16_t spilocal;

/* Debounce counts: Number spi time tick (~50ms) counts */
#define DB_INIT_SAFEACTIVE       6  // Switch pair: Safe|Active
#define DB_INIT_CP_REVERSETORQ   6  // Pushbutton:  CP_REVERSETORQ

/* Debounce type: */
#define DB_INIT_PB_1ST   0 // Show on immediately & off not before period expires
#define DB_INIT_PB_WAIT  1 // Wait until period expires.

uint32_t spitickctr; // Running count of spi time ticks (mostly for debugging)
uint32_t swxctr;

/* Pushbutton linked list head pointer. */
static struct SWITCHPTR* phdsw = NULL; // Pointer link head: switches instantiated
static struct SWITCHPTR* phddb = NULL; // Pointer link head: active debouncing

/* Array of pointers to instantiated switch structs, indexed by switch bit position. */
static struct SWITCHPTR* pchange[NUMSWS] = {0};

/* *************************************************************************
 * static void debouncing_add(struct SWITCHPTR* p);
 *	@brief	: Add 'this' switch to the list that is active debouncing.
 * @param	: p = pointer to struct for this pushbutton 
 * *************************************************************************/
static void debouncing_add(struct SWITCHPTR* p)
{
	struct SWITCHPTR* p1 = phddb; // Pointer head debounce list

	if (p1 == NULL)
	{ // Here, no switches are debouncing active
		phddb = p; // Point debounce head to this switch struct
		p->pdbnx = NULL; // This switch is the last on the list
	}
	else
	{ // Here, one or more on list
		while (p1->pdbnx != NULL)
		{
			p1 = p1->pdbnx; // Get next
		}
		// Here, p1 points to last on list
		p1->pdbnx = p;   // Add to list
		p->pdbnx = NULL; // Newly added item is now last
	}
	return;
}
/* *************************************************************************
 * static void debouncing_remove(struct SWITCHPTR* p);
 *	@brief	: Remove a pointer from list of active debouncing sws
 * @param	: p = pointer to struct for this pushbutton 
 * *************************************************************************/
static void debouncing_remove(struct SWITCHPTR* p)
{
	struct SWITCHPTR* p1 = phddb; // List head
	struct SWITCHPTR* p2;

	/* If we ask to remove, then it should be on the list! */
	if (p1 == NULL) morse_trap(521); // List was empty!

	/* Most likely situation. Just one item on list */
	if (p1->pdbnx == NULL)
	{ // First item on list should(!) be 'p'
		if (p1 != p) morse_trap(522);
		phddb = NULL; // Debounce list is now empty
		return;
	}

	/* Here, one or more on list. Search for 'this' sw. */
	do
	{
		p2 = p1; // Save  previous
		p1 = p1->pdbnx; // Get next
		if (p1 == p)
		{
			p2->pdbnx = p->pdbnx;
			return;
		}
	}	while (p1 != NULL);
	morse_trap(523);
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
	 uint8_t db_mode, 
	 uint8_t dur_closing,
	 uint8_t dur_opening);
 *
 *	@brief	: Add a single on/off (e.g. pushbutton) switch on a linked list
 * @param	: tskhandle = Task handle; NULL to use current task; 
 * @param	: notebit = notification bit; 0 = no notification
 * @param	: switchbit  = bit position in spi read word for this switch
 * @param	: switchbit1 = bit position for 2nd switch if a switch pair
 * @param	: db_mode = debounce mode: 0 = immediate; 1 = wait debounce
 * @param	: dur_closing = number of spi time tick counts for debounce
 * @param	: dur_opening = number of spi time tick counts for debounce
 * @return	: Pointer to struct for this switch
 * *************************************************************************/
struct SWITCHPTR* switch_pb_add(osThreadId tskhandle, uint32_t notebit, 
	 uint32_t switchbit,
	 uint32_t switchbit1,
	 uint8_t db_mode, 
	 uint8_t dur_closing,
	 uint8_t dur_opening)
{
	struct SWITCHPTR* p1 = phdsw;
	struct SWITCHPTR* p2;
	uint32_t n,nn;
	
taskENTER_CRITICAL();

	if (p1 == NULL)
	{ // Here, get first item for list
		p2 = (struct SWITCHPTR*)calloc(1, sizeof(struct SWITCHPTR));
		if (p2 == NULL){taskEXIT_CRITICAL(); morse_trap(524);}

		p1    = p2;		
		phdsw = p1;
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

	/* Active debounce linkage. */
	p2->pdbnx = NULL; // Not active debouncing

	/* Initialize linked list switch struct */
	if (tskhandle == NULL) // Use calling task handle?
		 tskhandle  = xTaskGetCurrentTaskHandle();
	p2->tskhandle  = tskhandle;
	p2->notebit    = notebit;   // Notification bit to use
	p2->switchbit  = switchbit; // Bit position in spi word
	p2->switchbit1 = switchbit1;// 2nd for switch pair
	p2->on         = 0;
	p2->db_on      = 0;
	p2->db_ctr     = 0;
	p2->state      = SWPB_CLOSED; // variables initially zero, so closed
	p2->db_dur_closing = dur_closing; // spi tick count
	p2->db_dur_opening = dur_opening; // spi tick count
	p2->db_mode        = db_mode;     // "now" or "after" debounce

	/* Map switch bit position (of xor'd changes) to this new struct. */
	n = arm_clz(p2->switchbit); // Count leading zeros
	if (n == 32) morse_trap(526);
	nn = 31 - n; // n=31 is bit position 0; n=0 is bit pos 31
	if (nn > (SPISERIALPARALLELSIZE*8)) morse_trap(527); 
	pchange[nn] = p2; // Bit position pointer to new struct
		
taskEXIT_CRITICAL();
	return p2;
}

/* *************************************************************************
 * static void pbtick(struct SWITCHPTR* p);
 *	@brief	: spi time tick debouncing for pushbutton type switches
 * @param	: p = pointer to struct for a pushbutton on the debounce linked list
 * *************************************************************************/
static void pbtick(struct SWITCHPTR* p)
{
	/* Ignore if debounce time complete. */
	if (p->db_ctr == 0) 
	{ // Here, must be the special zero-debouncing situation
		debouncing_remove(p);
		return;
	}

	/* Countdown debounce timing. */
	p->db_ctr -= 1;
	if (p->db_ctr != 0) return; // Still timing

	/* Here, the end of the debounce duration. */
	switch(p->state)
	{
	case SWPB_OPENING: // Switch is in process of opening
		if (p->on == SW_CLOSED)
		{ // Here, sw closed during a sw opening debounce period
			p->state = SWPB_CLOSED;
			if (p->db_mode == SW_NOW)
			{
				p->db_on = SW_CLOSED; // Show representation sw closed
				if (p->notebit != 0)
					xTaskNotify(p->tskhandle, p->notebit, eSetBits);
			}
		}
		else
		{ // Here, sw is open and state is opening (from closed)
			p->state = SWPB_OPEN; // Set constant open state
			if (p->db_mode == SW_WAITDB)
			{ // Here, mode is wait until debounce completes
				p->db_on = SW_OPEN;   // Show representation sw open
				if (p->notebit != 0)
					xTaskNotify(p->tskhandle, p->notebit, eSetBits);
			}
		}
		break;		

	case SWPB_CLOSING: // Switch is in process of closing
		if (p->on != SW_CLOSED)			
		{ // Here, sw opened during the debounce duration
			p->state = SWPB_OPEN; // We are back to open state
			if (p->db_mode == SW_NOW)
			{
				p->db_on = SW_OPEN;   // Show representation sw open
				if (p->notebit != 0)
					xTaskNotify(p->tskhandle, p->notebit, eSetBits);
			}
	//    else{ // mode is delay/wait, so db_on remains unchanged}
		}
		else
		{ // Here, debounce complete and sw still closed
			p->state = SWPB_CLOSED;				
			if (p->db_mode == SW_WAITDB)
			{
				p->db_on = SW_CLOSED;
				if (p->notebit != 0)
					xTaskNotify(p->tskhandle, p->notebit, eSetBits);
			}
		}
		break;
	}

	/* Remove this switch from the active debouncing list. */
	debouncing_remove(p);
	return;
}
/* *************************************************************************
 * static void pbxor(struct SWITCHPTR* p);
 *	@brief	: logic for pushbutton bit *changed*
 * @param	: p = pointer to struct for this pushbutton 
 * *************************************************************************/
static void pbxor(struct SWITCHPTR* p)
{
	/* Save present contact state: 1 = open, 0 = closed. */
	p->on = p->switchbit & spilocal; 

	switch(p->state)
	{
	case SWPB_OPEN: // Current debounce state: open
		if (p->on == SW_CLOSED)
		{ // Here, change: open state -> closed contact
			if (p->db_dur_closing == 0) // Debounce count?
			{ // Special case: instantaneous state change, no debounce
				p->db_on = SW_CLOSED;    // Debounced: representative of sw
				p->state = SWPB_CLOSED;  // New sw state
				if (p->notebit != 0)
					xTaskNotify(p->tskhandle, p->notebit, eSetBits);
			}
			else
			{ // One or more debounce ticks required.
				if (p->db_mode == SW_NOW)
				{ // Debounced sw representation shows new contact state "now"
					p->db_on = SW_CLOSED;
					if (p->notebit != 0) // Notify new state, if indicated
						xTaskNotify(p->tskhandle, p->notebit, eSetBits);
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
				if (p->notebit != 0)
					xTaskNotify(p->tskhandle, p->notebit, eSetBits);
			}
			else
			{ // One or more debounce ticks required.
				if (p->db_mode == SW_NOW)
				{
					p->db_on = SW_OPEN;
					if (p->notebit != 0)
						xTaskNotify(p->tskhandle, p->notebit, eSetBits);
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
 * static void spitick(void);
 *	@brief	: Debounce timing
 * *************************************************************************/
static void spitick(void)
{
		spitickctr += 1; // Mostly for Debugging & Test

/* =========== SWITCH PAIR: SAFE/ACTIVE logic. ========== */
		if (swpair_safeactive.state == SWP_DEBOUNCE)
		{ // Safe/Active is in debounce mode
			if (swpair_safeactive.db_ctr > 0)
			{ // Count down 
				swpair_safeactive.db_ctr -= 1;
				if (swpair_safeactive.db_ctr ==  0)
				{ // Debounce period has ended
					// From here-onward: Not 10 (safe off|active on) goes to safe mode
					swpair_safeactive.state = SWP_NO;
				}
			}
		}

/* ========== PUSHBUTTONS ========== */	
	/* Traverse linked list for pushbuttons actively debouncing. */
	struct SWITCHPTR* p = phddb;

	while (p != NULL)
	{
		pbtick(p); // Do The Debounce step
		p = p->pnext;
	}

	return;
}
/* *************************************************************************
 * static void swchanges(uint16_t spixor);
 *	@brief	: The queue'd word is not zero, meaning one or more sws changed
 * *************************************************************************/
static void swchanges(uint16_t spixor)
{
	uint16_t swb;

/* Not so elegant handling of a switch pair. */
/* =========== SAFE/ACTIVE logic. ========== */
	// Any changes to this switch pair?	
	if ((spixor & (SW_SAFE | SW_ACTIVE)) != 0) 
	{ // Here, yes.
		// New readings of both switches
		swb = spilocal & (SW_SAFE | SW_ACTIVE);
		switch (swpair_safeactive.state)
		{
		case SWP_UNDEFINED:
// NOTE: Remember switch making contact has the bit = 0
				// Look for SAFE on & ACTIVE off
				if (swb != SW_ACTIVE) break; 
				swpair_safeactive.state = SWP_NC;

		case SWP_NC: // We are in safe sw state
			if (!(swb == SW_SAFE)) // Not 10? 
				break; // break: 00 01 11  remain safe state

			// Here, sws are 10: safe sw is open (1) active closed (0)
			swpair_safeactive.db_ctr = DB_INIT_SAFEACTIVE;
			swpair_safeactive.state  = SWP_DEBOUNCE;
			swpair_safeactive.on     = SW_CLOSED;

			// Notify GevcuTask switches changed to ACTIVE
			xTaskNotify(GevcuTaskHandle, GEVCUBIT01, eSetBits);

		case SWP_DEBOUNCE: // Time tick counting in progress.
			if ((swb & SW_SAFE) == 0)
			{ // Here, safe sw made contact! Exit active state now.
				swpair_safeactive.state = SWP_NC;
				swpair_safeactive.on    = SW_OPEN;

				// Notify GevcuTask that switches changed to SAFE
				xTaskNotify(GevcuTaskHandle, GEVCUBIT02, eSetBits);
			}
			break;

		case SWP_NO: // In active state
				break; // break: 10 remain in active state

			// Here, 11,10,00, i.e. not (ACTIVE closed AND safe open)	
			swpair_safeactive.state = SWP_NC;
			swpair_safeactive.on    = SW_OPEN;
		
			// Notify GevcuTask that switches changed to SAFE
			xTaskNotify(GevcuTaskHandle, GEVCUBIT02, eSetBits);
			break;
		}
	}
/* ======== ON/OFF (pushbutton) CHANGES ===================== */
	uint32_t n,nn;            // Leading zeros, Bit positions
	uint32_t xortmp = spixor; // Working word of change bits
	struct SWITCHPTR* p;

	/* Deal with only switches that have a change. */
	while (xortmp != 0)
	{
		/* Get bit position of changed bit w asm instruction. */
		n = arm_clz(xortmp); // Count leading zeros
		if (n == 32) morse_trap(528); // Shouldn't happen

		nn = (31 - n); // n=31 is bit position 0; n=0 is bit pos 31
		if (nn >= (SPISERIALPARALLELSIZE*8)) morse_trap(529); 

		/* Point to struct for this switch */
		p = pchange[nn]; 

		/* Switches not instantiated have pullups and shows a 1. */
		if (p != NULL) // Skip switches not instantiated
		{	/* Update this switch */
			pbxor(p);
		}

		/* Clear change-bit for this switch */
		xortmp = (xortmp & ~(1 << nn) );
	}

	return;
}
/* *************************************************************************
 * void StartSwitchTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartSwitchTask(void* argument)
{
	BaseType_t qret;
	uint16_t spixor; // spi read-in difference

   /* Infinite loop */
   for(;;)
   {
		qret = xQueueReceive( SwitchTaskQHandle,&spixor,portMAX_DELAY);
		if (qret == pdPASS)
		{ // Here spi interrupt loaded a xor'd read-word onto the queue
			if (spixor == 0)
			{ // Here, spi interrupt countdown "time" tick
				spitick();
			}
			else
			{ // Deal with switch bit changes
				spilocal = (spilocal ^ spixor); // Update local copy
				swchanges(spixor);
			}
		}
   }
}
/* *************************************************************************
 * osThreadId xSwitchTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: SwitchTaskHandle, or NULL if queue or task creation failed
 * *************************************************************************/
osThreadId xSwitchTaskCreate(uint32_t taskpriority)
{
	BaseType_t ret = xTaskCreate(&StartSwitchTask, "SwitchTask",\
     80, NULL, taskpriority, &SwitchTaskHandle);
	if (ret != pdPASS) return NULL;

	SwitchTaskQHandle = xQueueCreate(SWITCHQSIZE, sizeof(uint16_t) );
	if (SwitchTaskQHandle == NULL) return NULL;

	return SwitchTaskHandle;
}


