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
#include "BeepTask.h"
#include "GevcuTask.h"
#include "SpiOutTask.h"
#include "gevcu_idx_v_struct.h"
#include "main.h"
#include "morse.h"
#include "getserialbuf.h"
#include "yprintf.h"
#include "lcdprintf.h"
#include "shiftregbits.h"
#include "spiserialparallelSW.h"
#include "LcdTask.h"
#include "LcdmsgsetTask.h"

#include "LcdTask.h"

/* Send position percent to LCD. */
#define SENDLCDPOSITIONTOUART

/* Timeout for re-issue LCD/Beep prompt */
#define CLTIMEOUT (128*1)  // 1 second

/* Timeout for re-sending LCD msg. */
#define CLRESEND (128/3)   // 3 per sec

/* GevcuTask counts 'sw1timer' ticks for various timeouts.
	 Gevcu polling timer (ka_k): 4 * (1/512), 128/sec
*/

//#define CLTIMEOUTTEST (128/4) // Test and debug
#define CLLINEDELAY (128/8)  // Time delay for 20 char line (9600 baud)

/* Uncomment to enable LCD position going to monitor uart. */
//#define SENDLCDPOSITIONTOUART

/* LCD splash screen delay. */
#define SPLASHDELAY (80) // (128 = 1 sec)
/* LCD delay following command */
#define LCDLINEDELAY (2) // 2 * (1/128) = 15.625 ms
#define INITDELAY2 ( 3)  // OFF (31.25 ms)
#define INITDELAY3 ( 3)  // ON  (31.25 ms)
#define INITDELAY4 (64)  // CLEAR (~400 ms)
#define INITDELAY5 ( 3)  // BACKLIGHT  (31.25 ms)
#define INITDELAY6 ( 7)  // MOVE CURSOR (~100 ms)
#define INITDELAY7 ( 2)  // Clear row with 20 spaces

static uint8_t clrrowctr = 0;

extern struct LCDI2C_UNIT* punitd4x20; // Pointer LCDI2C 4x20 unit

/* Flag to show others when CL calibration is complete. */
uint8_t flag_clcalibed; // 0 = CL not calibrated; 1 = CL calib complete

uint8_t flag_cllcdrdy;  // 0 = LCD not initialized; 1 = LCD OK to use

struct CLFUNCTION clfunc;

/* Beeper: { duration on, duration off, repeat count}; */
static const struct BEEPQ beep1 = {200,50,1}; // Close prompt
static const struct BEEPQ beep2 = {200,50,1}; // Full open prompt
static const struct BEEPQ beep3 = {100,40,1}; // Success beeping
static const struct BEEPQ beepf = { 60,20,1}; // We are waiting for your prompt

/* LCD output buffer pointers. */
static struct SERIALSENDTASKBCB* pbufmon1;    // HUARTMON (Monitor UART)
static struct SERIALSENDTASKBCB* pbuflcd1;    // HUART (LCD uart)
static struct LCDTASK_LINEBUF*   pbuflcdi2c1; // Ptr to buffer #1 LCDI2C unit 4x20
static struct LCDTASK_LINEBUF*   pbuflcdi2c2; // Ptr to buffer #2 CDLI2C unit 4x20

enum CLSTATE
{
	INITLCD,
	INITLCD1,
	INITLCD2,
	INITLCD3,
	INITLCD4,
	INITLCD5,
	INITLCD6,
	INITLCD7,
	CLOSE1,
	CLOSE1WAIT,
	CLOSE1MAX,
	OPEN1,
	OPEN1WAIT,
	OPEN1MAX,
	CLOSE2,
	CLOSE2WAIT,
	CLCREADY,   // CL calibration complete
	SEQDONE,
	SEQDONE1
};

struct SWITCHPTR* psw_cl_fs_no;
struct SWITCHPTR* psw_cl_rst_n0;

/* ***********************************************************************************************************
 * static void init(void);
 * @brief	: Prep for CL calibration
 ************************************************************************************************************* */
/*
This initialization is what would be in a file 'calib_control_lever_idx_v_struct.[ch]' if
the CL would become a separate function.
*/
static void init(void)
{
	clfunc.min    = 65521;
   clfunc.max    = 0;
	clfunc.toctr  = 0;
	clfunc.curpos = 0;
	clfunc.state  = INITLCD;

	clfunc.deadr = 2.25; // Deadzone for 0% (closed) boundary
	clfunc.deadf = 1.75; // Deadzone for 100% (open) boundary

	clfunc.hysteresis = 0.5; // Hysteresis pct +/- around curpos
	clfunc.curpos_h   = 1E5; // Impossible rest position (reading)

	clfunc.range_er = 25000; // Minimum range for calbirated CL

	/* LCD (uart) lcdprintf buffer */
	pbuflcd1 = getserialbuf(&HUARTLCD,32);
	if (pbuflcd1 == NULL) morse_trap(81);

#ifdef SENDLCDPOSITIONTOUART
   pbufmon1 = getserialbuf(&HUARTMON,48);
#endif

	/* Initialize switches for debouncing. */

	// Control Lever Fullscale Normally open. */
	psw_cl_fs_no = switch_pb_add(
		NULL,            /* task handle = this task    */
		0,               /* Task notification bit      */
		CL_FS_NO,        /* 1st sw see shiftregbits.h  */
		0,               /* 2nd sw (0 = not sw pair)   */
      SWTYPE_PB,       /* switch on/off or pair      */
	 	SWMODE_NOW,      /* Debounce mode              */
	 	SWDBMS(250),     /* Debounce ms: closing       */
	   SWDBMS(20));     /* Debounce ms: opening       */ 

	// Control Lever Fullscale Normally open. */
	psw_cl_rst_n0 = switch_pb_add(
		NULL,            /* task handle = this task    */
		0,               /* Task notification bit      */
		CL_RST_N0,       /* 1st sw see shiftregbits.h  */
		0,               /* 2nd sw (0 = not sw pair)   */
      SWTYPE_PB,       /* switch on/off or pair      */
	 	SWMODE_NOW,      /* Debounce mode              */
	 	SWDBMS(250),     /* Debounce ms: closing       */
	   SWDBMS(20));     /* Debounce ms: opening       */ 

	return;
}

/* ***********************************************************************************************************
 * void lcdout(void);
 * @brief	: Output CL calibrated position to LCD. NOTE: called from defaultTask after calibration completes.
 ************************************************************************************************************* */
/* LCDI2C 4x20 msg. */
static struct LCDMSGSET lcdmsgcl1;
static struct LCDMSGSET lcdmsgcl2;
static void lcdmsgfunc1(union LCDSETVAR u ){lcdi2cprintf(&pbuflcdi2c1,CLROW,0,"CL %5.1f%%  ",u.f);}
static void lcdmsgfunc2(union LCDSETVAR u ){lcdi2cprintf(&pbuflcdi2c2,CLROW,11,"%5d    ",u.u32);}

void lcdout(void)
{
	/* Do not update until calibration complete. */
	if (flag_clcalibed == 0) return; 

	/* Skip 'printf unless enough time has elapsed for a previous line. */
	if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0) // Pace output updates
	{ 
		if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0) return;

	 	lcdprintf(&pbuflcd1,CLROW,0,"CL %5.1f%% %5d     ",clfunc.curpos,adc1.abs[0].adcfil); // LCD
 		clfunc.timx = gevcufunction.swtim1ctr + CLLINEDELAY;
 		
 		/* Load the struct and place a pointer to it on a queue for execution by LcdmsgsetTask. */
 		// This avoids 'printf' semaphores stalling the program calling 'lcdout'
	 	lcdmsgcl1.u.f = clfunc.curpos; // Value that is passed to function 
  		lcdmsgcl1.ptr = lcdmsgfunc1;   // Pointer to the function

  		if (LcdmsgsetTaskQHandle != NULL)
    		xQueueSendToBack(LcdmsgsetTaskQHandle, &lcdmsgcl1, 0);

    	// Since only one value is passed via the queue the second value to be displayed is
    	// setup by a second msg. A second buffer is used to avoid stalling in the lcd output
    	// routine when the first is in-progress sending. That stalling is not really important
    	// since LcdTask is a separate task, however.
    	lcdmsgcl2.u.u32 = adc1.abs[0].adcfil; // Value that is passed to function 
  		lcdmsgcl2.ptr = lcdmsgfunc2;   // Pointer to the function

  		if (LcdmsgsetTaskQHandle != NULL)
    		xQueueSendToBack(LcdmsgsetTaskQHandle, &lcdmsgcl2, 0);
	}
}
/* ***********************************************************************************************************
 * static void lcdclearrow(uint8_t row);
 * @brief	: Output CL position
 * @param	: row = row number (0 - 3)
 ************************************************************************************************************* */
static void lcdclearrow(uint8_t row)
{
	/* Clear a row string.       
//                20 spaces:  01234567890123456789*/
	lcdprintf(&pbuflcd1,row,0,"                    ");
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
	float ftmp;
	float fcur;
	float frange;

		/* LCD (I2C) lcdi2cprintf buffer. */
		/* We can't do this in the 'init' since the LCD may not have been instantiated. */
	uint32_t loopctr = 0;
	    while ((punitd4x20 == NULL) && (loopctr++ < 10)) osDelay(10);
  	    if (punitd4x20 == NULL) morse_trap(2326);

		if (pbuflcdi2c1 == NULL)
		    pbuflcdi2c1 = xLcdTaskintgetbuf(punitd4x20, 32);
		if (pbuflcdi2c1 == NULL) morse_trap(2327);				

		if (pbuflcdi2c2 == NULL)
		    pbuflcdi2c2 = xLcdTaskintgetbuf(punitd4x20, 32);
		if (pbuflcdi2c2 == NULL) morse_trap(2328);				


		switch (clfunc.state)
		{ // The following is a one-time only sequenceSPLASHDELAY
		case INITLCD: // Half-second delay for splash screen to clear
			init(); // #### Initialize ####
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
			clfunc.timx = gevcufunction.swtim1ctr + INITDELAY7;
			clfunc.state = INITLCD7; // NEXT: CL forward

		case INITLCD7: // Clear all for rows of LCD
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break; // Still waiting for timeout
			if (clrrowctr < 4) // Do all four rows
			{
				lcdclearrow(clrrowctr); // Send 20 spaces to clear line
				clfunc.timx = gevcufunction.swtim1ctr + CLLINEDELAY;
				clrrowctr += 1; // Advance row number
				break;
			}

			/* Let others know the LCD initialization sequence is complete. */
			flag_cllcdrdy = 1;

		/* === LCD initialization complete === */

		/* Lever calbiration: Move lever to stops and collect ADC readings. */
		case OPEN1:	
			
			/* Both limit switches should not be ON at the same time! */
			if ((psw_cl_fs_no->on == 0) && (psw_cl_rst_n0->on == 0))	
			{  //                           01234567890123456789  
				lcdi2cputs(&pbuflcdi2c1,CLROW,0,"CL ERR: BOTH SWS ON ");
				lcdprintf (&pbuflcd1,   CLROW,0,"CL ERR: BOTH SWS ON ");
				xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);

				clfunc.state = INITLCD1;
				clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT*1; 		
				break;
			}		                     // "...................." 
			lcdi2cprintf(&pbuflcdi2c1,CLROW,0,"FULL FWD LEVER %5d  ",clfunc.toctr++);
			lcdprintf   (&pbuflcd1,   CLROW,0,"FULL FWD LEVER %5d  ",clfunc.toctr  );
//			xQueueSendToBack(BeepTaskQHandle,&beep2,portMAX_DELAY);
			clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT;
			clfunc.state = OPEN1WAIT;
			break;

		case OPEN1WAIT:
//			if ((spisp_rd[0].u16 & CL_FS_NO) != 0) // Non-debounced
			if (psw_cl_fs_no->db_on != 0) // Debounced
			{ // Here, sw for forward position is not closed
				if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0)
				{ // Time out waiting. Alert Op again.
//					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
					clfunc.state = OPEN1; // Timed out--re-beep the Op
				}
				break;
			}
			clfunc.state = OPEN1MAX;
			clfunc.timx = gevcufunction.swtim1ctr + CLRESEND;
			// Drop through to OPEN1MAX

		case OPEN1MAX:
			// Here, forward sw is closed. Save ADC readings until sw opens
			if ((float)adc1.abs[0].adcfil > clfunc.max)
			{ // Save new and larger reading
				clfunc.max = (float)adc1.abs[0].adcfil;
			}
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0)
			{
				lcdi2cputs(&pbuflcdi2c1,CLROW,0,"CLOSE LEVER         ");
				lcdprintf (&pbuflcd1,   CLROW,0,"CLOSE LEVER    %5d  ",clfunc.toctr++);
				clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT;
			}
//			if ((spisp_rd[0].u16 & CL_FS_NO) == 0) // Non-debounced
			if (psw_cl_fs_no->db_on == 0) // Debounced
				break;				
			// Here, sw for forward position has gone open

		/* CL is moving to rest postion. */
		case CLOSE1:
//			xQueueSendToBack(BeepTaskQHandle,&beep1,portMAX_DELAY);
			clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT;
			clfunc.state = CLOSE1WAIT;
//			break;

		case CLOSE1WAIT:
//			if ((spisp_rd[0].u16 & CL_RST_N0) != 0) Non-debounced
			if (psw_cl_rst_n0->db_on != 0)
			{ // Here, sw for rest position is not closed
				if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0)
				{
//					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
 					lcdi2cputs(&pbuflcdi2c1,CLROW,0,"CLOSE LEVER         ");
					lcdprintf (&pbuflcd1,   CLROW,0,"CLOSE LEVER    %5d  ",clfunc.toctr++);
					clfunc.state = CLOSE1; // Timed out--re-beep the Op
				}
				break;
			}
			clfunc.state = CLOSE1MAX;
			clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT;

		/* Find minimum reading. */
		case CLOSE1MAX:
			// Here, rest position sw is closed. Save ADC readings until sw opens
			if ((float)adc1.abs[0].adcfil < clfunc.min)
			{ // Save new and larger reading
				clfunc.min = (float)adc1.abs[0].adcfil;
			}
//			if ((spisp_rd[0].u16 & CL_RST_N0) == 0) // Non-debounced
			if (psw_cl_rst_n0->db_on == 0)
				clfunc.state = SEQDONE;
				break;

			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) < 0)
			{
				lcdi2cputs(&pbuflcdi2c1,CLROW,0,"CLOSE LEVER         ");
				lcdprintf (&pbuflcd1,   CLROW,0,"CLOSE LEVER!   %5d  ",clfunc.toctr++);
				clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT; 
			}
			break;

			/* === CL ADC readings have been determined. === */

		case SEQDONE: // Calibration computations
			/* Total travel of CL in terms of ADC readings. */
			frange = (clfunc.max - clfunc.min);

			/* Sanity check. */
			if (frange < clfunc.range_er)
			{
				lcdi2cputs(&pbuflcdi2c1,CLROW,0,"CL RANGE ERROR      ");			
				lcdprintf (&pbuflcd1,   CLROW,0,"CL RANGE ERROR %0.1f",frange);
				xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
				clfunc.state = INITLCD1;
				clfunc.timx = gevcufunction.swtim1ctr + CLTIMEOUT*1; 		
				break;		
			}

			/* ADC reading below minends will be set to zero. */
			clfunc.minends   = (clfunc.min + frange * (float)0.01 * clfunc.deadr);

			/* ADC readings above maxbegins will be set to 100.0 */
			clfunc.maxbegins = (clfunc.max - frange * (float)0.01 * clfunc.deadf);

			/* This scales the effective range for the CL */ 			
			clfunc.rcp_range = (float)100.0/(clfunc.maxbegins - clfunc.minends);

			/* Compute reading difference (above/below) to trigger new current position. */
			clfunc.h_diff = 0.01 * clfunc.hysteresis * (clfunc.maxbegins - clfunc.minends);

			/* Some fluff for the Op. */
//			xQueueSendToBack(BeepTaskQHandle,&beep3,portMAX_DELAY);

			lcdclearrow(CLROW); // Send 20 spaces to clear line
			clfunc.timx = gevcufunction.swtim1ctr + CLLINEDELAY;
			clfunc.state = SEQDONE1;

		case SEQDONE1: // Let line clearing complete
			if ((int)(clfunc.timx - gevcufunction.swtim1ctr) > 0)
				break;

			clfunc.state = CLCREADY;

		/* === Calibration is complete. === */

		/* Compute position of CL. */
		case CLCREADY:
			/* New adc reading. */
			fcur = adc1.abs[0].adcfil; // Convert to float

			/* Large absolute change triggers change in curpos. */
			ftmp = fcur - clfunc.curpos_h; // (New - Current) position
			if (ftmp < 0) ftmp = -ftmp; // Make absolute
			if (ftmp > clfunc.h_diff)   // Does change exceed limit?
			{ // Here, change beyond hysteresis boundary

				/* Save current position (in terms of readings). */
				clfunc.curpos_h = fcur; // Update position (readings)

				/* Are we in the low end dead zone? */
				if (fcur < clfunc.minends)
				{ // New reading is in the closed deadzone
					clfunc.curpos =  0; // Force 0.0%
//					lcdout(); // Output to LCD
					flag_clcalibed = 1; // Set calibrated flag
					break;	// Done
				}
				else
				{ // Here, not in zero dead zone.

					/* Are we in the max end dead zone? */
					if (fcur > clfunc.maxbegins)
					{ // New reading is in the full forward deadzone
						clfunc.curpos = (float)100.0; // Force 100.0%
//						lcdout(); // Output to LCD
						flag_clcalibed = 1; // Set calibrated flag
						break;	// Done
					}
					else
					{ // Here, fcur is greater than 0.0 zone, and less than 100.0 zone. */
						// Compute curpos in terms of pct
						clfunc.curpos = (fcur - clfunc.minends) * clfunc.rcp_range;			
//						lcdout(); // Output to LCD
						flag_clcalibed = 1; // Set calibrated flag
						break;	// Done
					}
				}
			}
			break;

		// Program Gone Wild trap
		default: morse_trap (80);
		}
	return clfunc.curpos;
}

