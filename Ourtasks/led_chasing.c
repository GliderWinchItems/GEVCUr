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

#include "SwitchTask.h"
#include "spiserialparallel.h"
#include "SpiOutTask.h"
#include "shiftregbits.h"
#include "GevcuTask.h"
#include "morse.h"
#include "calib_control_lever.h"
#include "LEDTask.h"

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

/* Map of LED sequence for CP panel. */
static const uint8_t ledmap[16] = 
{
	LED_STOP,
	LED_ABORT,
	LED_RETRIEVE,
	LED_RECOVERY,
	LED_CLIMB,
	LED_RAMP,
	LED_GNDRLRTN,
	LED_ARM,
	LED_PREP,
	LED_SAFE,
	LED_ARM_PB,
	LED_PREP_PB,
	LED_SPARERS, 
	LED_SPARE10,  
	LED_SPARE08, 
	LED_SPAREFS, 
};

struct LEDREQ spiledx      = {LED_STOP   ,0}; // Current LED ON
struct LEDREQ spiledx_prev = {LED_SPAREFS,1}; // Previous LED (to be turned OFF)

static uint8_t chasectr = 0; // Counter for slowing down output rate
static uint8_t seqctr   = 0; // (0 - 15) sequence

/* *************************************************************************
 * void led_chasing(void);
 *	@brief	: Step through LEDs until CL calibrates
 * *************************************************************************/
void led_chasing(void)
{
	switch (flag_clcalibed)
	{
	case 0:
		chasectr += 1;
		if (chasectr > 1)
		{
			chasectr = 0;
			/* Send a lit LED down the row, over and over. */
			spiledx.mode = LED_ON; // Turn current LED on
			xQueueSendToBack(LEDTaskQHandle,&spiledx,portMAX_DELAY);

			spiledx_prev.mode = LED_OFF; // Turn previous LED off
			xQueueSendToBack(LEDTaskQHandle,&spiledx_prev,portMAX_DELAY);
			
			spiledx_prev = spiledx; // Update previous
	
			/* Step to next LED to be displayed. */
			seqctr += 1;   // Advance sequence ctr
			if (seqctr >= 16) seqctr = 0;
			spiledx.bitnum = ledmap[seqctr]; // Map LED
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
