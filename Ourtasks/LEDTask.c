/******************************************************************************
* File Name          : LEDTask.c
* Date First Issued  : 01/31/2020
* Description        : SPI/LED control
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"

#include "LEDTask.h"
#include "spiserialparallelSW.h"
#include "SpiOutTask.h"
#include "shiftregbits.h"
#include "morse.h"

/* Define to do bone head when linked list not working! */
// Comment out to use linked list method
#define USELEDLISTARRAYSCAN

osThreadId LEDTaskHandle = NULL;
osMessageQId LEDTaskQHandle;

/* Each LED gets one of these. */
/*
{ // Bit number, on/off set to off. 
#define LED_STOP        14 //
#define LED_ABORT       15 //
#define LED_RETRIEVE     0 //
#define LED_RECOVERY     1 //
#define LED_CLIMB        2 //
#define LED_RAMP         3 //
#define LED_GNDRLRTN     4 //
#define LED_ARM          5 //
#define LED_PREP         6 //
#define LED_SAFE         7 //
#define LED_ARM_PB       8 //
#define LED_PREP_PB      9 //
#define LED_SPARERS     10 // *LED_CL_RST
#define LED_SPARE11     11 //
#define LED_SPARE12     12 //
#define LED_SPARE13     13 // *LED_CL_FS
*/         

struct LEDCTL
{
	uint16_t bitmsk; // spi bit mask (1 << led_bitnum)
	uint8_t mode; // Mode: off,on,blink...
	uint8_t ctr;  // Timing counter
	uint8_t on;   // Present on/off status
};

/* Linked list for LEDs currently in blink mode. */
// next = NULL: Last on linked list
// next = 1: Not on linked list
// next = not above: points to next on linked list
// prev = points to struct that points to us.
// prev = NULL 1st item on list
// phead = NULL: list empty
// phead = not NULL: points to first on list
#define NOTONLIST 1 // 'next' value for not on linked list
struct LEDLIST
{
	struct LEDLIST* next;
	struct LEDLIST* prev;
	struct LEDCTL ctl;
};

/* Entry for each possible LED. */
static struct LEDLIST* phead;
static struct LEDLIST ledlist[17];

/* Preset counter ticks for blink modes. */
static const uint16_t dur_off[] = 
{ /* Blinking: OFF timing count. */
	 0,
	 0,
	32, /* Blink slow */
	 5, /* Blink fast */
	64, /* Blink 1sec */
   64  /* Blink short wink */
};
static const uint16_t dur_on[] = 
{ /* Blinking: ON timing count. */
	 0,
	 0,
	32, /* Blink slow */
	 3, /* Blink fast */
	64, /* Blink 1sec */
    8  /* Blink short wink */
};

osMessageQId LEDTaskQHandle;

/* *************************************************************************
 * static void init(void);
 *	@brief	: Initialize LED struct array
 * *************************************************************************/
static void init(void)
{
	uint8_t i;
	for (i = 0; i < 16; i++)
	{
		ledlist[i].ctl.mode   = 0;
		ledlist[i].ctl.ctr    = 0;
		ledlist[i].ctl.on     = 0;
		ledlist[i].ctl.bitmsk = (1 << i);
		ledlist[i].next = (struct LEDLIST*)NOTONLIST; // Not on list
		ledlist[i].prev = NULL; // List link
	
	}
	spisp_wr[0].u16 = 0;    // Set spi LED bits off
	phead           = NULL; // Empty list
	return;
}
/* *************************************************************************
 * static void blink_init(struct LEDLIST* p, uint8_t mode);
 *	@brief	: Initialize for blinking
 * @param	: p = pointer to LED struct item to be blinked
 * @param	: mode = blink mode code
 * *************************************************************************/


static void blink_init(struct LEDLIST* p, uint8_t mode)
{
#ifdef USELEDLISTARRAYSCAN
/* Scan array method. */
	p->ctl.mode  = mode; // Update blink mode
	p->ctl.ctr   = dur_on[mode]; // Init 1st duration counter
	spisp_wr[0].u16 |= p->ctl.bitmsk; // Set LED on
	return;

#else
/* Linked list method. */
	struct LEDLIST* p2;

	/* If currently on list nothing needs to be done. */
	if (p->next != (struct LEDLIST*)NOTONLIST)
	{ // Here, this LED is already on the linked list
		// It could be a shift to a different type of blink
		return;
	}
	/* Here, p is not on the list. */
	if (phead == NULL) // List empty?
	{ // Yes, p is 1st and also last on list
		phead = p;   // head points to p
		p->next = NULL; // p is also last on list
		p->prev = NULL; // phead is previous
	}
	else
	{	/* Add entry to head of list. */
		p2        = phead; // Save current ptr to 1st entry
		phead     = p;     // Head now pts to new entry
		p->next   = p2;    // Point to next in list (could be NULL)
		p->prev   = NULL;  // Previous for new entry is always phead
		p2->prev  = p;     // New entry now precedes old 1st entry
	}

	/* Init this LED. */
	p->ctl.mode  = mode; // Update blink mode
	p->ctl.ctr   = dur_on[mode]; // Init 1st duration counter
	spisp_wr[0].u16 |= p->ctl.bitmsk; // Set LED on

	return;
#endif
}
/* *************************************************************************
 * static void blink(void);
 *	@brief	: Do the blinking
 * *************************************************************************/
static void blink(void)
{
	struct LEDLIST* p1 = phead;

#ifdef USELEDLISTARRAYSCAN
/* Go through array looking and handle active blinkers. */

	p1 = &ledlist[0];	
	while (p1 != &ledlist[16])
	{ // Here, p1 points to an active LED. */
		if (p1->ctl.mode > 1)
	 {
		// Timing counter
		if (p1->ctl.ctr != 0)
		{ // Countdown, and stay in current led state.
			p1->ctl.ctr -= 1;
		}
		else
		{ // Here, ctr is at zero. Update led state
			if (p1->ctl.on == 0)
			{ // Here, led is off. Set led on & set on time ct.
				spisp_wr[0].u16 |= p1->ctl.bitmsk; // Set spi bit on
				p1->ctl.on = LED_ON; // LED was set to on
				// Init duration counter of on.
				p1->ctl.ctr = dur_on[p1->ctl.mode];
			}
			else
			{ // Here, it is on. Set off & set off time ct.
				spisp_wr[0].u16 &= ~(p1->ctl.bitmsk); // Set spi bit off
				p1->ctl.on = LED_OFF; // LED was set to off
				// Init duration counter for off
				p1->ctl.ctr = dur_off[p1->ctl.mode];
			}
		}
    }
	 p1 += 1;
	}
	return;

#else /* Linked list method. */

	if (p1 == NULL) return; // List empty

if (p1 == (struct LEDLIST*)NOTONLIST) morse_trap(292);

	/* Traverse linked list looking for active blinkers. */
	do
	{ // Here, p1 points to an active LED. */
		if (p1->ctl.mode > 1)  // Needed?, or morse_trap it?
	 	{ // Here, one of the blinking modes
			// Timing counter
			if (p1->ctl.ctr != 0)
			{ // Countdown, and stay in current led state.
				p1->ctl.ctr -= 1;
			}
			else
			{ // Here, ctr is at zero. Update led state
				if (p1->ctl.on == 0)
				{ // Here, led is off. Set led on & set on time ct.
					spisp_wr[0].u16 |= p1->ctl.bitmsk; // Set spi bit on
					p1->ctl.on = LED_ON; // LED was set to on
					// Init duration counter of on.
					p1->ctl.ctr = dur_on[p1->ctl.mode];
				}
				else
				{ // Here, it is on. Set off & set off time ct.
					spisp_wr[0].u16 &= ~(p1->ctl.bitmsk); // Set spi bit off
					p1->ctl.on = LED_OFF; // LED was set to off
					// Init duration counter for off
					p1->ctl.ctr = dur_off[p1->ctl.mode];
				}
			}
		}
		p1 = p1->next; // 'next' is NULL on last item on list
	} while (p1 != NULL); 

	return;
#endif
}
/* *************************************************************************
 * static void blink_cancel(struct LEDLIST* p);
 *	@brief	: Cancel blinking, if in a blink mode
 * @param	: p = pointer into struct array 
 * *************************************************************************/
static void blink_cancel(struct LEDLIST* p)
{
#ifdef USELEDLISTARRAYSCAN
/* Array scan method : updating 'mode' effectively cancels blinking. */
	return;
#else
/* Linked list method: remove struct from linked list. */

	struct LEDLIST* p2 = NULL;
	struct LEDLIST* p1 = phead;

	if (p->next == (struct LEDLIST*)NOTONLIST)
		return;

	if (p1 != NULL)
	{ // Here list is not empty
		/* Find previous in linked list to this one. */
		p2 = p->prev;

		if (p2 == NULL)
		{ // Here, removal of p will make an empty list
			phead = NULL;
			p->prev = NULL;
		}
		else
		{
			if (p->next == NULL)
			{ // Last on list
				p2->next = p->next; // (NULL)
			}
			else
			{ // Not last on list
				p1 = p->next;
				p1->prev = p->prev;
				p2->next = p->next;  // Remove from list
			}
		}
	}
	p->next = (struct LEDLIST*)NOTONLIST; // Not on list	
	return;
#endif
}
/* *************************************************************************
 * void StartLEDTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartLEDTask(void* argument)
{
	struct LEDREQ ledreq;
	struct LEDLIST* p;
	BaseType_t qret;
	uint8_t i;

  /* Infinite loop */
  for(;;)
  {
		/* Timing loop. */
		osDelay(pdMS_TO_TICKS(16)); // Timing using FreeRTOS tick counter

		/* Unload all LED change requests on queue */
		do
		{ 
			qret = xQueueReceive(LEDTaskQHandle,&ledreq,0);
			if (qret == pdPASS)
			{ // Here, led request was on the queue
				i = ledreq.bitnum; // Convenience variable
				if (i > 15) morse_trap(86); // Bad queue bitnum (ignorant programmer)
				p = &ledlist[i]; // More convenience and speed
				
				/* Start the mode requested. */
				switch (ledreq.mode)
				{ 
				case LED_OFF: // LED off
					blink_cancel(p);
					p->ctl.mode = ledreq.mode; // Update mode for this led
					spisp_wr[0].u16 &= ~(1 << i); // Set LED off
					break;

				case LED_ON: // LED on
					blink_cancel(p);
					p->ctl.mode = ledreq.mode; // Update mode for this led
					spisp_wr[0].u16 |= (1 << i); // Set LED on
					break;

				// Initialize blinking
				case LED_BLINKSLOW:
					blink_init(p,LED_BLINKSLOW);
					break;

				case LED_BLINKFAST: 
					blink_init(p,LED_BLINKFAST);
					break;

				case LED_BLINK1SEC:
					blink_init(p,LED_BLINK1SEC);
					break;

				case LED_BLINKWINK:
					blink_init(p,LED_BLINKWINK);
					break;

				default:
					morse_trap(87); // Programmer goofed: mode req not in range.
					break;
				}
			}
		} while (qret == pdPASS); // Continue until queue empty

		/* Blink LEDs that are on blink list */
		blink();
	}
}
/* *************************************************************************
 * osThreadId xLEDTaskCreate(uint32_t taskpriority, uint32_t ledqsize);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @param	: ledqsize = number of items allowable on queue
 * @return	: LEDTaskHandle
 * *************************************************************************/
osThreadId xLEDTaskCreate(uint32_t taskpriority, uint32_t ledqsize)
{
	init(); // Initialized led struct array

	BaseType_t ret = xTaskCreate(&StartLEDTask, "LEDTask",\
     128, NULL, taskpriority, &LEDTaskHandle);
	if (ret != pdPASS) return NULL;

	LEDTaskQHandle = xQueueCreate(ledqsize, sizeof(struct LEDREQ) );
	if (LEDTaskQHandle == NULL) return NULL;

	return LEDTaskHandle;
}


