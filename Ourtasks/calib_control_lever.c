/******************************************************************************
* File Name          : calib_control_lever.c
* Date First Issued  : 01/26/2020
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
#include "lcdprintf.h"

/* Send position percent to LCD. */
#define SENDLCDPOSITIONTOUART

/* Timeout for re-issue LCD/Beep prompt */
#define CLTIMEOUT (128*5)  // 5 seconds

/* GevcuTask counts 'sw1timer' ticks for various timeouts.
	 Gevcu polling timer (ka_k): 4 * (1/512), 128/sec
*/

#define CLTIMEOUTTEST (128/4) // Test and debug

/* Uncomment to enable LCD position going to monitor uart. */
//#define SENDLCDPOSITIONTOUART

/* LCD splash screen delay. */
#define SPLASHDELAY (128 * 3)  // 3 seconds
/* LCD delay following commant */
#define LCDLINEDELAY (2) // 2 * (1/128) = 15.625 ms
#define INITDELAY2 ( 4)  // OFF (31.25 ms)
#define INITDELAY3 ( 4)  // ON  (31.25 ms)
#define INITDELAY4 (51)  // CLEAR (~400 ms)
#define INITDELAY5 ( 4)  // BACKLIGHT  (31.25 ms)
#define INITDELAY6 (13)  // MOVE CURSOR (~100 ms)

/* Flag to show others when CL calibration is complete. */
uint8_t flag_clcalibed; // 0 = CL not calibrated; 1 = CL calib complete

struct CLFUNCTION clfunc;

/* Beeper: { duration on, duration off, repeat count}; */
static const struct BEEPQ beep1 = {200,50,1}; // Close prompt
static const struct BEEPQ beep2 = {200,50,1}; // Full open prompt
static const struct BEEPQ beep3 = {100,40,2}; // Success beeping
static const struct BEEPQ beepf = {60,20,2};  // We are waiting for you prompt

/* uart output buffers. */
static struct SERIALSENDTASKBCB* pbuflcd1;
static struct SERIALSENDTASKBCB* pbufmon1;

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
	clfunc.min    = 65521;
   clfunc.max    = 0;
	clfunc.toctr  = 0;
	clfunc.curpos = 0;
	clfunc.state  = INITLCD;

	clfunc.deadr = 3.0; // Deadzone for 0% (closed)
	clfunc.deadf = 3.0; // Deadzone for 100% (open)

	/* lcdprintf buffer */
	pbuflcd1 = getserialbuf(&HUARTLCD,32);
	if (pbuflcd1 == NULL) morse_trap(81);

#ifdef SENDLCDPOSITIONTOUART
   pbufmon1 = getserialbuf(&HUARTMON,48);
#endif

	return;
}

/* ***********************************************************************************************************
 * float calib_control_lever(void);
 * @brief	: Calibrate CL
 * @param	: control lever postion as a percent (0 - 100)
 * @return	: Return lever position as percent
 ************************************************************************************************************* */
uint32_t ccldbg;

float calib_control_lever(void)
{
	uint8_t byt[32];
	uint8_t* p;


	float fcur;
	float frange;

		switch (clfunc.state)
		{ // The following is a one-time only sequenceSPLASHDELAY
		case INITLCD: // Half-second delay for splash screen to clear
			clfunc.timx = gevcufunction.swtim1ctr + SPLASHDELAY;
			clfunc.state = INITLCD1;
//			lcdprintf_init(&pbuflcd1);	// Do init sequence in one uart line
			break;

		case INITLCD1: // Delay further LCD commands until time expires.
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;
			p = lcd_off(&byt[0]); // LCD OFF
			*p = 0;
			lcdputs(&pbuflcd1, (char*)&byt[0]);
			clfunc.timx = gevcufunction.swtim1ctr + INITDELAY2;
			clfunc.state = INITLCD2;
			break;

		case INITLCD2: 
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;
			p = lcd_on(&byt[0]); // LCD ON
			*p = 0;
			lcdputs(&pbuflcd1, (char*)&byt[0]);
			clfunc.timx = gevcufunction.swtim1ctr + INITDELAY3;
			clfunc.state = INITLCD3;
			break;

		case INITLCD3: 
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;
			p = lcd_clear(&byt[0]); // LCD CLEAR
			*p = 0;
			lcdputs(&pbuflcd1, (char*)&byt[0]);
			clfunc.timx = gevcufunction.swtim1ctr + INITDELAY4;
			clfunc.state = INITLCD4;
			break;

		case INITLCD4: 
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;
			p = lcd_backlight(&byt[0], LCD_BACKLIGHT_LEVEL); // LCD Backlight
			*p = 0;
			lcdputs(&pbuflcd1, (char*)&byt[0]);
			clfunc.timx = gevcufunction.swtim1ctr + INITDELAY5;
			clfunc.state = INITLCD5;
			break;

		case INITLCD5: 
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;
			p = lcd_moveCursor(&byt[0],0,0); // LCD MOVE CURSOR
			*p = 0;
			lcdputs(&pbuflcd1, (char*)&byt[0]);
			clfunc.timx = gevcufunction.swtim1ctr + INITDELAY6;
			clfunc.state = INITLCD6; // NEXT: CL forward
			break;

		case INITLCD6: // Complete wait of MOVE CURSOR
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;

		case OPEN1:			
			lcdprintf(&pbuflcd1,1,0,"FULL FWD LEVER %d  ",clfunc.toctr++);
			xQueueSendToBack(BeepTaskQHandle,&beep2,portMAX_DELAY);
			clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT;
			clfunc.state = OPEN1WAIT;
			break;

		case OPEN1WAIT:
			if ((spisp_rd[0].u16 & CL_FS_NO) != 0)
			{ // Here, sw for forward position is not closed
				if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0)
				{ // Time out waiting. Alert Op again.
					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
					clfunc.state = OPEN1; // Timed out--re-beep the Op
				}
				break;
			}
			clfunc.state = OPEN1MAX;
			// Drop through to OPEN1MAX
		case OPEN1MAX:
			// Here, forward sw is closed. Save ADC readings until sw opens
			if ((float)adc1.chan[0].sum > clfunc.max)
			{ // Save new and larger reading
				clfunc.max = (float)adc1.chan[0].sum;
			}
			if ((spisp_rd[0].u16 & CL_FS_NO) == 0) break;				
			// Here, sw for forward position has gone open

		/* Move CL to rest postion. */
		case CLOSE1:
			lcdprintf(&pbuflcd1,1,0,"CLOSE LEVER %d ",clfunc.toctr++);
			xQueueSendToBack(BeepTaskQHandle,&beep1,portMAX_DELAY);
			clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT;
			clfunc.state = CLOSE1WAIT;
			break;

		case CLOSE1WAIT:
			if ((spisp_rd[0].u16 & CL_RST_N0) != 0)
			{ // Here, sw for rest position is not closed
				if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0)
				{
					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
					clfunc.state = CLOSE1; // Timed out--re-beep the Op
				}
				break;
			}
			clfunc.state = CLOSE1MAX;

		/* Move CL to rest position. */
		case CLOSE1MAX:
			// Here, rest position sw is closed. Save ADC readings until sw opens
			if ((float)adc1.chan[0].sum < clfunc.min)
			{ // Save new and larger reading
				clfunc.min = (float)adc1.chan[0].sum;
			}
			if ((spisp_rd[0].u16 & CL_RST_N0) == 0) break;
			// Here, sw for rest position has gone open
	
		case SEQDONE:
			/* Total travel of CL in terms of ADC readings. */
			frange = (clfunc.max - clfunc.min);

			/* ADC reading below minends will be set to zero. */
			clfunc.minends   = (clfunc.min + frange*(float)0.01*clfunc.deadr);

			/* ADC readings above maxbegins will be set to 100.0 */
			clfunc.maxbegins = (clfunc.max - frange*(float)0.01*clfunc.deadf);

			/* This scales the effective range for the CL */ 			
			clfunc.rcp_range = (float)100.0/(clfunc.maxbegins - clfunc.minends);

			/* Some coding fluff for the Op. */
//			lcdprintf(&pbuflcd1,1,0,"SUCCESS        ");
			xQueueSendToBack(BeepTaskQHandle,&beep3,portMAX_DELAY);
			clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUTTEST; // Time delay for lcd printf
			clfunc.state = CLCREADY;
			flag_clcalibed = 1; // Let others know CL is calibrated
//			break;
		
		/* Calibration is complete. Compute position of CL. */
		case CLCREADY:
			fcur = adc1.chan[0].sum;
			if (fcur < clfunc.minends)
			{ // CL is in the closed deadzone
				clfunc.curpos =  0;

#ifdef SENDLCDPOSITIONTOUART
if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0){
//  yprintf(&pbufmon1,"\n\rCL %5.1f %5d",clfunc.curpos,adc1.chan[0].sum);
  lcdprintf(&pbuflcd1,1,0,"CL %5.1f %5d  ",clfunc.curpos,adc1.chan[0].sum);
  clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUTTEST;
}
#endif
				break;
			}
			if (fcur > clfunc.maxbegins)
			{
				clfunc.curpos = (float)100.0;

#ifdef SENDLCDPOSITIONTOUART
if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0){
//  yprintf(&pbufmon1,"\n\rCL %5.1f %5d",clfunc.curpos,adc1.chan[0].sum);
  lcdprintf(&pbuflcd1,1,0,"CL %5.1f %5d  ",clfunc.curpos,adc1.chan[0].sum);
  clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUTTEST;
}
#endif
				break;
			}
			clfunc.curpos = (fcur - clfunc.minends) * clfunc.rcp_range;

#ifdef SENDLCDPOSITIONTOUART
if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0){
//  yprintf(&pbufmon1,"\n\rCL %5.1f %5d",clfunc.curpos,adc1.chan[0].sum);
  lcdprintf(&pbuflcd1,1,0,"CL %5.1f %5d  ",clfunc.curpos,adc1.chan[0].sum);
  clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUTTEST;
}
#endif
			break;
			
		// Program Gone Wild trap
		default: morse_trap (80);
		}
	return clfunc.curpos;
}

