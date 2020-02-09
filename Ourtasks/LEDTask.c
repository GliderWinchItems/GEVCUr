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
#include "spiserialparallel.h"
#include "SpiOutTask.h"
#include "shiftregbits.h"
#include "morse.h"

osThreadId LEDTaskHandle = NULL;
osMessageQId LEDTaskQHandle;

/* Each LED gets one of these. */
/*
static struct SPIOUTREQUEST spiout[] = 
{ // Bit number, on/off set to off. 
 {LED_STOP      , 0}, //    0 //
 {LED_ABORT     , 0}, //    1 //
 {LED_RETRIEVE  , 0}, //    2 //
 {LED_RECOVERY  , 0}, //    3 //
 {LED_CLIMB     , 0}, //    4 //
 {LED_RAMP      , 0}, //    5 //
 {LED_GNDRLRTN  , 0}, //    6 //
 {LED_ARM       , 0}, //    7 //
 {LED_PREP      , 0}, //    8 //
 {LED_SAFE      , 0}, //    9 //
 {LED_ARM_PB    , 0}, //   10 //
 {LED_PREP_PB   , 0}, //   11 //
 {LED_SPARERS   , 0}, //   12 // *LED_CL_RST
 {LED_SPARE10   , 0}, //   13 //
 {LED_SPARE08   , 0}, //   14 //
 {LED_SPAREFS   , 0}, //   15 // *LED_CL_FS
};
*/         

struct LEDCTL
{
	uint16_t bitmsk; // spi bit mask (1 << led_bitnum)
	uint8_t mode; // Mode: off,on,blink...
	uint8_t ctr;  // Timing counter
	uint8_t on;   // Present on/off status
};

/* Linked list for LEDs currently in blink mode. */
struct LEDLIST
{
	struct LEDLIST* next;
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
	64, /* Blink slow */
	16, /* Blink fast */
	64, /* Blink 1sec */
  112  /* Blink short wink */
};
static const uint16_t dur_on[] = 
{ /* Blinking: ON timing count. */
	 0,
	 0,
	64, /* Blink slow */
	 4, /* Blink fast */
	64, /* Blink 1sec */
   16  /* Blink short wink */
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
		ledlist[i].next = NULL; // List link
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
	struct LEDLIST* p2;

	/* Extending blinking if currently active. */
	if (p->next != NULL)
	{ // Here, this LED is on the linked list
		p->ctl.ctr = dur_on[mode]; // Re-init count
		return;
	}

	/* Add entry to head of list. */
	p2        = phead; // Save ptr
	phead     = p;     // Head pts to this struct
	p->next   = p2;    // Point to next in list

	/* Init this LED. */
	p->ctl.mode  = mode; // Update blink mode
	p->ctl.ctr   = dur_on[mode]; // Init 1st duration counter
	spisp_wr[0].u16 |= p->ctl.bitmsk; // Set LED on

	return;
}
/* *************************************************************************
 * static void blink(void);
 *	@brief	: Do the blinking
 * *************************************************************************/
static void blink(void)
{
	struct LEDLIST* p1 = phead;

	/* Traverse linked list looking for active blinkers. */
	while (p1 != NULL)
	{ // Here, p1 points to an active LED. */
		
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
		p1 = p1->next;
	}
	return;
}
/* *************************************************************************
 * static void blink_cancel(struct LEDLIST* p);
 *	@brief	: Cancel blinking, if in a blink mode
 * @param	: p = pointer into struct array 
 * *************************************************************************/
static void blink_cancel(struct LEDLIST* p)
{
	struct LEDLIST* p1 = phead;
	struct LEDLIST* p2 = NULL;

	/* Check if this LED was in a blink mode. */
	switch (p->ctl.mode)
	{
	case LED_BLINKSLOW:
	case LED_BLINKFAST:
	case LED_BLINK1SEC:
	// Here, it is in one of the blink modes.

		/* Find previous in linked list to this one. */
		while (p1 != p)
		{
			p2 = p1;
			p1 = p1->next;
			if (p1 >= &ledlist[16]) morse_trap(29);
		}
		if (p2 == NULL)
		{
			phead = NULL;
		}
		else
		{
			p2->next = p->next;  // Remove from list
			p->next = 0;	// Show not active
		}
	}
	return;
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


