/******************************************************************************
* File Name          : LEDTask.c
* Date First Issued  : 01/31/2020
* Description        : GSM Control Panel LED 
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
	uint8_t mode;
	uint8_t ctr;    // Timing counter
	uint8_t onoff;  // Present on/off status
};

/* Specification for each possible LED/ */
static struct LEDCTL ledctl[16] = {0};

/* Preset counter ticks for blink modes. */
static const uint16_t dur_off[] = 
{ /* Blinking: OFF timing count. */
	0,
	0,
	64, /* Blink slow */
	16, /* Blink fast */
	128 /* Blink 1sec */
};
static const uint16_t dur_on[] = 
{ /* Blinking: ON timing count. */
	0,
	0,
	64, /* Blink slow */
	16, /* Blink fast */
	128 /* Blink 1sec */
};

osMessageQId LEDTaskQHandle;

/* *************************************************************************
 * void StartLEDTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartLEDTask(void* argument)
{
	struct LEDREQ ledreq;
	BaseType_t qret;
	uint8_t i;


  /* Infinite loop */
  for(;;)
  {
		/* Timing loop. */
		osDelay(pdMS_TO_TICKS(4)); // Timing using FreeRTOS tick counter

		do
		{ // Unload all LED change requests on queue
			qret = xQueueReceive(LEDTaskQHandle,&ledreq,0);
			if (qret == pdPASS)
			{ // Here, led request was on the queue
				i = ledreq.bitnum; // Convenience variable
				if (i > 15) morse_trap(86); // Bad queue bitnum
				ledctl[i].mode = ledreq.mode; // Save mode for this led

				switch (ledctl[i].mode)
				{ // Deal with the mode requested
				case LED_OFF: // LED off
					spisp_wr[0].u16 &= ~(1 << i); // Set off
					break;

				case LED_ON: // LED on
					spisp_wr[0].u16 |= (1 << i); // Set on
					break;

				// Blinking modes.
				case LED_BLINKSLOW:
				case LED_BLINKFAST: 
				case LED_BLINK1SEC:
					// Timing counter
					if (ledctl[i].ctr != 0)
					{ // Countdown, and stay in current led state.
						ledctl[i].ctr -= 1;
					}
					else
					{ // Here, ctr is at zero. Update led state
						if (ledctl[i].onoff == 0)
						{ // Here, led is off. Set led on & set on time ct.
							spisp_wr[0].u16 |= (1 << i); // Set on
							// Set off duration counter
							ledctl[i].ctr = dur_on[ledctl[i].mode];
						}
						else
						{ // Here, it is on. Set off & set off time ct.
							spisp_wr[0].u16 &= ~(1 << i); // Set off
							// Set on duration counter
							ledctl[i].ctr = dur_off[ledctl[i].mode];
						}
					}
					break;

				default:
					morse_trap(87); // Programmer goofed: mode req not in range.
					break;
				}
			}
		} while (qret == pdPASS);
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
	BaseType_t ret = xTaskCreate(&StartLEDTask, "LEDTask",\
     64, NULL, taskpriority, &LEDTaskHandle);
	if (ret != pdPASS) return NULL;

	LEDTaskQHandle = xQueueCreate(ledqsize, sizeof(struct LEDREQ) );
	if (LEDTaskQHandle == NULL) return NULL;

	return LEDTaskHandle;
}


