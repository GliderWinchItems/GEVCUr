/******************************************************************************
* File Name          : led_chasing.c
* Date First Issued  : 02/11/2020
* Description        : stepping through leds 
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"

#include "spiserialparallelSW.h"
#include "SpiOutTask.h"
#include "shiftregbits.h"
#include "GevcuTask.h"
#include "morse.h"
#include "calib_control_lever.h"
#include "LEDTask.h"
#include "BeepTask.h"


/* email: Tue, 11 Feb 2020 03:14:30 +0000 (02/10/2020 10:14:30 PM)

The order of the LED confirmation sequence is now:

Stop
Abort
Retrieve
Recovery
Climb
Ramp
Groundroll/Rotation
Arm
Prep
Save
Arm PB
Prep PB

02/23/2020 Desired seq:
Safe
Prep
Armed
Groundroll
Ramp
Climb
Recovery
Prep
Retrieve
Abort
Stop
Prep PB
Arm PB

 Sixteen LEDs from gsm CP tests. 
#define LED_STOP         0 //
#define LED_ABORT        1 //
#define LED_RETRIEVE     2 //
#define LED_RECOVERY     3 //
#define LED_CLIMB        4 //
#define LED_RAMP         5 //
#define LED_GNDRLRTN     6 //
#define LED_ARM          7 //
#define LED_PREP         8 //
#define LED_SAFE         9 //
#define LED_ARM_PB      10 //
#define LED_PREP_PB     11 //
#define LED_SPARERS     12 // *LED_CL_RST
#define LED_SPARE10     13 //
#define LED_SPARE08     14 //
#define LED_SPAREFS     15 // *LED_CL_FS
*/
/* 02/23/2020 Intended chase sequence
Safe
Prep
Armed
Groundroll
Ramp
Climb
Recovery
Prep
Retrieve
Abort
Stop
Prep PB
Arm PB
*/
/* Map of LED sequence for CP panel. */
static const uint8_t ledmap[] = 
{
	LED_SAFE,
	LED_PREP,
	LED_ARM,
	LED_GNDRLRTN,
	LED_RAMP,
	LED_CLIMB,
	LED_RECOVERY,
	LED_PREP,
	LED_RETRIEVE,
	LED_ABORT,
	LED_STOP,
	LED_PREP_PB,
	LED_ARM_PB,
	0xFF	/* End of sequence */
};

struct LEDREQ spiledx      = {LED_SAFE   ,0}; // Current LED ON
struct LEDREQ spiledx_prev = {LED_ARM_PB ,1}; // Previous LED (to be turned OFF)

static uint8_t chasectr = 0; // Counter for slowing down output rate
static uint8_t seqctr   = 0; // (0 - 15) sequence

static uint8_t led_chasing_state = 0;
static uint8_t allonctr = 0;
static const struct BEEPQ beep1 = {200,300,2}; // End of sequence

/* *************************************************************************
 * void led_chasing(void);
 *	@brief	: Step through LEDs until CL calibrates
 * *************************************************************************/
void led_chasing(void)
{
	switch (flag_clcalibed)
	{
	case 0:

		switch (led_chasing_state)
		{
		case 0: // Turn all LEDs on
			spisp_wr[0].u16 = 0xffff; // Set all LEDs ON
			chasectr = 0;
			led_chasing_state = 1;
			break;

		case 1: // Time all LEDs on
			chasectr += 1; // Timing counter
			if (chasectr <=10) break;

			// Turn all LEDs off and time
			spisp_wr[0].u16 = 0x0000; // Set all LEDs OFF
			chasectr = 0;
			led_chasing_state = 2;
			break;

		case 2:
			chasectr += 1; // Timing counter
			if (chasectr <= 10) break;
			seqctr = 0;
			spiledx.bitnum = ledmap[0]; // First one in sequence
			spiledx_prev.bitnum = ledmap[2]; // Use not the first one
			chasectr = 0;
			allonctr += 1;
			if (allonctr > 1)
				led_chasing_state = 3;
			else
				led_chasing_state = 0;
			break;

		case 3:
			chasectr += 1; // Timing counter
			if (chasectr > 1)
			{ // Next step
				chasectr = 0; // Reset time counter

				/* Send a lit LED down the row, over and over. */
				spiledx.mode = LED_ON; // Turn current LED on
				xQueueSendToBack(LEDTaskQHandle,&spiledx,portMAX_DELAY);
	
				spiledx_prev.mode = LED_OFF; // Turn previous LED off
				xQueueSendToBack(LEDTaskQHandle,&spiledx_prev,portMAX_DELAY);
				
				spiledx_prev = spiledx; // Update previous
		
				/* Step to next LED to be displayed. */
				seqctr += 1;   // Advance sequence ctr
				if (ledmap[seqctr] == 0xFF) 
				{
					xQueueSendToBack(BeepTaskQHandle,&beep1,portMAX_DELAY);
					seqctr = 0;
					led_chasing_state = 4;
					break;
				}
				spiledx.bitnum = ledmap[seqctr]; // Map LED
			}
			break;

		case 4:
			chasectr += 1; // Timing counter
			if (chasectr > 1)
			{
				chasectr = 0; // Reset time counter
				spiledx_prev.mode = LED_OFF; // Turn previous LED off
				xQueueSendToBack(LEDTaskQHandle,&spiledx_prev,portMAX_DELAY);
				led_chasing_state = 2;
			}
			break;

		}
		break;

	case 1:
	// When Calibration complete turn off the latest lit LED
		if (flag_clcalibed == 1)
		{
			flag_clcalibed = 2; // We are done until next reboot
			spiledx_prev.mode = LED_OFF; // Turn previous LED off
			xQueueSendToBack(LEDTaskQHandle,&spiledx_prev,portMAX_DELAY);
		}
		break;
	}

	return;
}
