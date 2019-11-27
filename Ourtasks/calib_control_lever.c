/******************************************************************************
* File Name          : calib_control_lever.c
* Date First Issued  : 08/31/2014
* Board              : DiscoveryF4
* Description        : Master Controller: Control Lever calibration
*******************************************************************************/

#include "calib_control_lever.h"
#include <string.h>
#include "4x20lcd.h"
#include "adcparams.h"
#include "GevcuTask.h"
#include "shiftregbits.h"
#include "SpiOutTask.h"
#include "BeepTask.h"
#include "lcdprintf.h"
#include "gevcu_idx_v_struct.h"
#include "SpiOutTask.h"
#include "main.h"
#include "morse.h"
#include "getserialbuf.h"
#include "yprintf.h"

enum CLSTATE
{
	CLOSE1,
	CLOSE1WAIT,
	OPEN1,
	OPEN1WAIT,
	CLCREADY   // CL calibration complete
};

#define CLTIMEOUT (64*15)  // 1.5 seconds

struct CLFUNCTION clfunc;

/* Beeper: { duration on, duration off, repeat count}; */
static const struct BEEPQ beep1 = {100,50,1}; // Close prompt
static const struct BEEPQ beep2 = {100,50,2}; // Full open prompt
static const struct BEEPQ beep3 = {100,50,3}; // Success beeping
static const struct BEEPQ beepf = {60,40,6};  // We are waiting for you prompt

/* Skip for now
static const struct SPIOUTREQUEST ledcloseoff = {LED_CL_RST,0};
static const struct SPIOUTREQUEST ledcloseon  = {LED_CL_RST,1};
static const struct SPIOUTREQUEST ledopenoff  = {LED_CL_FS ,0};
static const struct SPIOUTREQUEST ledopenon   = {LED_CL_FS ,1};
*/

static struct SERIALSENDTASKBCB* pbuflcd;

/* ***********************************************************************************************************
 * void calib_control_lever_init();
 * @brief	: Prep for CL calibration
 ************************************************************************************************************* */
/*
This initialization is what would be in a file 'calib_control_lever_idx_v_struct.[ch]' if
the CL would become a separate function.
*/
void calib_control_lever_init()
{
	clfunc.state = 0;
	clfunc.min   = 0;
   clfunc.max   = 0;

	clfunc.state = 0;

	clfunc.deadr = 1.5; // Deadzone for 0% (closed)
	clfunc.deadf = 1.0; // Deadzone for 100% (open)

	/* lcdprintf buffer */
extern uint8_t lcdflag;
while(lcdflag == 0) osDelay(3);
yprintf_init();

	pbuflcd = getserialbuf(&HUARTLCD,32);
	if (pbuflcd == NULL) morse_trap(81);

	return;
}

/* ***********************************************************************************************************
 * void calib_control_lever(void);
 * @brief	: Calibrate CL
 ************************************************************************************************************* */
void calib_control_lever(void)
{
	float fcur;
	float frange;

		switch (clfunc.state)
		{ // The following is a one-time only sequence
		case CLOSE1:
//			lcdprintf(&pbuflcd,2,0,"CLOSE LEVER");
			xQueueSendToBack(BeepTaskQHandle,&beep1,portMAX_DELAY);
			clfunc.timx = gevcufunction.tasktim + CLTIMEOUT;
			clfunc.state = CLOSE1WAIT;
			break;

		case CLOSE1WAIT:
			if ((spisp_rd[0].u16 & CL_RST_N0) == 0)
			{ // Here, sw for rest position is not closed
				if ((int)(clfunc.timx - gevcufunction.tasktim) > 0)
				{
					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
					clfunc.state = CLOSE1; // Timed out--re-beep the Op
					break;
				}
			}
			// Here, switch is closed, so save ADC reading
			clfunc.min = (float)adc1.chan[0].sum;

		case OPEN1:			
//			lcdprintf(&pbuflcd,2,0,"OPEN LEVER");
			xQueueSendToBack(BeepTaskQHandle,&beep2,portMAX_DELAY);
			clfunc.timx = gevcufunction.tasktim + CLTIMEOUT;
			clfunc.state = CLOSE1WAIT;
			break;
		case OPEN1WAIT:
			if ((spisp_rd[0].u16 & CL_FS_NO) == 0)
			{ // Here, sw for rest position is not closed
				if ((int)(clfunc.timx - gevcufunction.tasktim) > 0)
				{
					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
					clfunc.state = OPEN1; // Timed out--re-beep the Op
					break;
				}
				break;
			}
			// Here, switch is closed, so save ADC reading
			clfunc.max = (float)adc1.chan[0].sum;
		
			/* Total travel of CL in terms of ADC readings. */
			frange = (clfunc.max - clfunc.min);

			/* ADC reading below minends will be set to zero. */
			clfunc.minends   = (clfunc.min + frange*(float)0.01*clfunc.deadr);

			/* ADC readings above maxbegins will be set to 100.0 */
			clfunc.maxbegins = (clfunc.max - frange*(float)0.01*clfunc.deadf);

			/* This scales the effective range for the CL */ 			
			clfunc.rcp_range = (float)100.0/(clfunc.maxbegins - clfunc.minends);

			/* Some coding fluff for the Op. */
//			lcdprintf(&pbuflcd,2,0,"SUCCESS");
			xQueueSendToBack(BeepTaskQHandle,&beep3,portMAX_DELAY);
			clfunc.state = CLCREADY;
			break;
		
		/* Calibration is complete. Compute position of CL. */
		case CLCREADY:
			fcur = adc1.chan[0].sum;
			if (fcur < clfunc.minends)
			{ // CL is in the closed deadzone
				clfunc.curpos =  0;
				break;
			}
			if (fcur > clfunc.maxbegins)
			{
				clfunc.curpos = (float)100.0;
				break;
			}
			clfunc.curpos = (fcur - clfunc.minends) * clfunc.rcp_range;
			break;
			
		// Program Gone Wild trap
		default: morse_trap (80);
		}

	return;
}

