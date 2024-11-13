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
#include "DTW_counter.h"
#include "common_can.h"
#include "gevcu_idx_v_struct.h"
#include "can_iface.h"
#include "common_can.h"

extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block

void send_can_clcp(void);

/* GevcuTask counts 'sw1timer' ticks for various timeouts.
	 Gevcu polling timer (ka_k): 4 * (1/512), 128/sec
*/

#define T1S (168000000)     // Number of DTW ticks in one second
#define T1MS (T1S/1000)     // Number of DTW ticks in one millisecond

/* LCD splash screen delay. */
#define SPLASHDELAY  (T1MS*500) 
/* LCD delay following command */
#define LCDLINEDELAY (T1MS* 20)  // 20 ms
#define INITDELAY2   (T1MS* 20)  // OFF (32 ms)
#define INITDELAY3   (T1MS* 20)  // ON  (32 ms)
#define INITDELAY4   (T1MS* 20)  // CLEAR (~400 ms)
#define INITDELAY5   (T1MS* 20)  // BACKLIGHT  (32 ms)
#define INITDELAY6   (T1MS* 20)  // MOVE CURSOR (~100 ms)
#define INITDELAY7   (T1MS* 40)  // Clear row with 20 spaces
#define CLTIMEOUT    (T1MS*800)  // Timeout for re-issue LCD/Beep prompt 
#define CLRESEND     (T1MS*800)  // Timeout for re-sending LCD msg.
#define CLLINEDELAY  (T1MS* 80)  // Time delay for 20 char line (9600 baud)

static uint8_t clrrowctr = 1;

extern struct LCDI2C_UNIT* punitd4x20; // Pointer LCDI2C 4x20 unit

/* Flag to show others when CL calibration is complete. */
uint8_t flag_clcalibed; // 0 = CL not calibrated; 1 = CL calib complete

/* Flag to tell led_chasing.c that CL calibration started. */
uint8_t flag_clcalibed_started;

uint8_t flag_cllcdrdy;  // 0 = LCD not initialized; 1 = LCD OK to use

/* CP status with CL setting. */
#define CPCANMSGRATE_SAFE 5000
#define CPCANMSGRATE_ACTIVE 250
uint16_t ratectr; // Throttle CAN msg rate
static struct CANTXQMSG cpclcan; // CP & CL status CAN msg


struct CLFUNCTION clfunc;

/* Beeper: { duration on, duration off, repeat count}; */
static const struct BEEPQ beep1 = {200,50,1}; // Close prompt
static const struct BEEPQ beep2 = {200,50,1}; // Full open prompt
static const struct BEEPQ beep3 = {100,40,1}; // Success beeping
static const struct BEEPQ beepf = { 60,20,1}; // We are waiting for your prompt

struct SWITCHPTR* psw_cl_fs_no;
struct SWITCHPTR* psw_cl_rst_n0;

/* ***********************************************************************************************************
 * void calib_control_lever_init(void);
 * @brief	: Prep for CL calibration
 ************************************************************************************************************* */
/*
This initialization is what would be in a file 'calib_control_lever_idx_v_struct.[ch]' if
the CL would become a separate function.
*/
void calib_control_lever_init(void)
{
	clfunc.min    = 65521;
   clfunc.max     = 0;
	clfunc.toctr  = 0;
	clfunc.curpos = 0;
	clfunc.state  = INITLCD;

	clfunc.deadr = 2.25; // Deadzone for 0% (closed) boundary
	clfunc.deadf = 1.75; // Deadzone for 100% (open) boundary

	clfunc.hysteresis = 0.5; // Hysteresis pct +/- around curpos
	clfunc.curpos_h   = 1E5; // Impossible rest position (reading)

	clfunc.range_er = 25000; // Minimum range for calbirated CL

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

		/* Initialize fixed data in CP/CL status CAN msg. */
		cpclcan.pctl       = pctl0; // CAN1 control block ptr (from main.c)
		cpclcan.maxretryct = 8;
		cpclcan.bits       = 0; 
		// CANID_HB_CPSWSCLV1_5: U8_U8_U16_U16_S16, CP:status:spare:sw err:sws on/off:CL +/- 10000');
		cpclcan.can.id     = 0x32000000;
		cpclcan.can.dlc    = 8;
		cpclcan.can.cd.ull = 0; // Clear payload
		ratectr = CPCANMSGRATE_SAFE/2; // Initial status msg 

	return;
}

/* ***********************************************************************************************************
 * void lcdout(void);
 * @brief	: Output CL calibrated position to LCD. NOTE: called from defaultTask after calibration completes.
 ************************************************************************************************************* */
/* LCDI2C 4x20 msg. */
static struct LCDMSGSET lcdmsgcl1;
//#define TWOCALLSWITHONEARGUMENT 
  #ifdef TWOCALLSWITHONEARGUMENT 	
static struct LCDMSGSET lcdmsgcl2;
static void lcdmsgfunc1(union LCDSETVAR u ){lcdi2cprintf(&punitd4x20,CLROW,0,"CL %5.1f%%  ",u.f);}
static void lcdmsgfunc2(union LCDSETVAR u ){lcdi2cprintf(&punitd4x20,CLROW,11,"%5d    ",u.u32);}
  #else
static void lcdmsgfunc3(union LCDSETVAR u ){lcdi2cprintf(&punitd4x20,CLROW,0,"CL %5.1f%%   adc%5d",u.ftwo[0],u.u32two[1]);}
  #endif

void lcdout(void)
{
	/* Do not update until calibration complete. */
	if (flag_clcalibed == 0) return; 

	/* Skip LCD msg when contactor fault msgs need to be displayed. */
	if ((lcdcontext & LCDX_CNTR) != 0) return;

/* Note: pacing of this is because it is called from defaultTask loop */	
#ifdef TWOCALLSWITHONEARGUMENT 		
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
#else

 	lcdmsgcl1.u.ftwo[0]   = clfunc.curpos; // float that is passed to function 
  lcdmsgcl1.u.u32two[1] = adc1.abs[0].adcfil; // u32 that is passed to function 
	lcdmsgcl1.ptr = lcdmsgfunc3;   // Pointer to the function

	if (LcdmsgsetTaskQHandle != NULL)
   		xQueueSendToBack(LcdmsgsetTaskQHandle, &lcdmsgcl1, 0);
#endif

	return;
}
/* ***********************************************************************************************************
 * static void lcdclearrow(uint8_t row);
 * @brief	: Output CL position
 * @param	: row = row number (0 - 3)
 ************************************************************************************************************* */
static void lcdclearrow(uint8_t row)
{
	/* Clear a row:               01234567890123456789*/
	lcdi2cputs(&punitd4x20,row,0,"                    ");
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
	float ftmp;
	float fcur;
	float frange;
	uint32_t loopctr;

		switch (clfunc.state)
		{ // The following is a one-time only sequenceSPLASHDELAY
		case INITLCD: // Half-second delay for splash screen to clear

		/* LCD (I2C) lcdi2cprintf buffer. */
		/* We can't do this in the 'init' since the LCD may not have been instantiated. */
		loopctr = 0;
	    while ((punitd4x20 == NULL) && (loopctr++ < 10)) osDelay(10);
  	    if (punitd4x20 == NULL) morse_trap(233);

//			init(); // #### Initialize ####
			clfunc.timx = DTWTIME + SPLASHDELAY;
			clfunc.state = INITLCD7; // NEXT: CL forward			

		case INITLCD7: // Clear all for rows of LCD
			if ((int)(clfunc.timx - DTWTIME) > 0)
				break; // Still waiting for timeout
			if (clrrowctr < 4) // Do all four rows
			{
				lcdclearrow(clrrowctr); // Send 20 spaces to clear line
				// Time delay between each row
				clfunc.timx = DTWTIME + CLLINEDELAY;
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
				lcdi2cputs(&punitd4x20,CLROW,0,"CL ERR: BOTH SWS ON ");

				if ((int)(clfunc.timx - DTWTIME) > 0)
					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
				clfunc.timx = DTWTIME + CLTIMEOUT*1; 		
				break;
			}
			lcdi2cprintf(&punitd4x20,CLROW,0,"FULL FWD LEVER %5d",clfunc.toctr++);
//			xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
			clfunc.timx = DTWTIME + CLTIMEOUT;
			clfunc.state = OPEN1WAIT;
			break;

		case OPEN1WAIT:
//			if ((spisp_rd[0].u16 & CL_FS_NO) != 0) // Non-debounced
			if (psw_cl_fs_no->db_on != 0) // Debounced
			{ // Here, sw for forward position is not closed
				if ((int)(clfunc.timx - DTWTIME) < 0)
				{ // Time out waiting. Alert Op again.
//					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
					clfunc.state = OPEN1; // Timed out--re-beep the Op
				}
				break;
			}
			clfunc.state = OPEN1MAX;
			clfunc.timx = DTWTIME + CLRESEND;
			flag_clcalibed_started = 1; // Let led_chasing know calib started
			// Drop through to OPEN1MAX

		case OPEN1MAX:
			// Here, forward sw is closed. Save ADC readings until sw opens
			if ((float)adc1.abs[0].adcfil > clfunc.max)
			{ // Save new and larger reading
				clfunc.max = (float)adc1.abs[0].adcfil;
			}
			if ((int)(clfunc.timx - DTWTIME) < 0)
			{
				lcdi2cprintf(&punitd4x20,CLROW,0,"CLOSE LEVER    %5d",clfunc.toctr++);
				clfunc.timx = DTWTIME + CLTIMEOUT;
			}
			if (psw_cl_fs_no->db_on == 0) // Debounced
				break;				
			// Here, sw for forward position has gone open

		/* CL is moving to rest postion. */
		case CLOSE1:
//			xQueueSendToBack(BeepTaskQHandle,&beep1,portMAX_DELAY);
			clfunc.timx = DTWTIME + CLTIMEOUT;
			clfunc.state = CLOSE1WAIT;
//			break;

		case CLOSE1WAIT:
//			if ((spisp_rd[0].u16 & CL_RST_N0) != 0) Non-debounced
			if (psw_cl_rst_n0->db_on != 0)
			{ // Here, sw for rest position is not closed
				if ((int)(clfunc.timx - DTWTIME) < 0)
				{
//					xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
 					lcdi2cprintf(&punitd4x20,CLROW,0,"CLOSE LEVERa   %5d",clfunc.toctr++);
					clfunc.state = CLOSE1; // Timed out--re-beep the Op
				}
				break;
			}
			clfunc.state = CLOSE1MAX;
			clfunc.timx = DTWTIME + CLTIMEOUT;

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

			if ((int)(clfunc.timx - DTWTIME) < 0)
			{
				lcdi2cprintf(&punitd4x20,CLROW,0,"CLOSE LEVERb   %5d",clfunc.toctr++);  
				clfunc.timx = DTWTIME + CLTIMEOUT; 
			}
			break;

			/* === CL ADC readings have been determined. === */

		case SEQDONE: // Calibration computations
			/* Total travel of CL in terms of ADC readings. */
			frange = (clfunc.max - clfunc.min);

			/* Sanity check. */
			if (frange < clfunc.range_er)
			{                                // 01234567890123456789
				lcdi2cputs(&punitd4x20,CLROW,0,"CL RANGE ERROR      ");			
				xQueueSendToBack(BeepTaskQHandle,&beepf,portMAX_DELAY);
				clfunc.timx = DTWTIME + CLTIMEOUT*2; 		
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
			clfunc.timx = DTWTIME + CLLINEDELAY;
			clfunc.state = SEQDONE1;

		case SEQDONE1: // Let line clearing complete
			if ((int)(clfunc.timx - DTWTIME) > 0)
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
					lcdcontext |= LCDX_CLIB; // Flag for LCD msg priority
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

			// Program Gone Wild trap; state not in list
			default: morse_trap (80);
		}

	send_can_clcp();

	return clfunc.curpos;
}
/* ***********************************************************************************************************
 * void send_can_clcp(void);
 * @brief	: Output CL status, position, and instantiated switches. Throttle output rate
 ************************************************************************************************************* */
/*
CP status msg--
'CANID_HB_CPSWSCLV1_5','32000000','CP',      1,1,'U8_U8_U16_U16_S16','HB_CPSWSV1 5:status:spare:sw err:sws on/off:CL +/- 10000');
'U8_U8_U16_U16_S16'  ,49,7,'[0]:uint8_t,[1]:uint8_t,[2:3]:uint16_t,[4:5]:uint16_t,[6:7]:int16_t');
pay[0] - status
    bit[7:2] = reserved
		bit[1] 0 = no errs in sws; 1 = one or more sws have err, or are indeterminate
    bit[0] 0 = not calibrated; 1 = CL calibrated;
pay[1] = reserverd
pay[2:3] = bit position for sws with err status
pay[4:5] = bit position for sw on/off (1 = off)
pay[6:7] = CL position percent * 100: +/- 10000 (100.00%)

Switch position mapping--
0 - Safe/Active switch pair after dpdt logic applied
1 - Prep
2 - ARM
3 - Zero Odometer
4 - Zero Tension
5 - CL full foward
6 - CL full rest
7 - 15 reserved :Sw positions not reported default to OPEN

*/
void send_can_clcp(void)
{
	/* Throttle rate according state. */
	ratectr += 1;
	switch(gevcufunction.state)
	{
	case	GEVCU_INIT: // Slow rate
	case	GEVCU_SAFE_TRANSITION:
	case 	GEVCU_SAFE:
		if (ratectr >= CPCANMSGRATE_SAFE)
		{
			ratectr = 0;
		}
		break;

	default: // Fast rate
		if (ratectr >= CPCANMSGRATE_ACTIVE)
		{
			ratectr = 0;
		}
		break;
	}
	if (ratectr == 0)
	{ // Output status msg

		/* pay[0] Status byte. */
		cpclcan.can.cd.uc[0]  = flag_clcalibed; // 0 = not calibrated, 1 = calibrated
		/* SAFE/ACTIVE sw is presently the only switch that can logically be
		   indeterminate so we only have to check it for status. The others are
		   naturally either on or off. */
		// SAFE/ACTIVE is dpdt: 0 err; 1 safe; 2 active; 3 err  
		if ((gevcufunction.psw[PSW_PR_SAFE]->on == 3) || (gevcufunction.psw[PSW_PR_SAFE]->on == 0))
			cpclcan.can.cd.uc[0] |= 0x2; // Error

		/* pay[1] = spare. */
		cpclcan.can.cd.uc[1]  = 0; // Spare

		/* pay[2:3] Switch err status. Bit position shows which sw has error. */
		// SAFE/ACTIVE is dpdt: 0 err; 1 safe; 2 active; 3 err  
		if ((gevcufunction.psw[PSW_PR_SAFE]->db_on & 0x2) != 0)
			cpclcan.can.cd.us[1] = 1;
		else
			cpclcan.can.cd.us[1] = 0;
			
		/* pay[4:5] Bits for switch on/off (0 = CLOSED; 1 = OPEN) */
		cpclcan.can.cd.us[2]  = 0xFF80; // Sw positions not reported default to OPEN

		if (gevcufunction.psw[PSW_PR_SAFE]->db_on == 1)
			cpclcan.can.cd.us[2] |= 0x1 << 0;

		cpclcan.can.cd.us[2] |= (gevcufunction.psw[PSW_PB_PREP]->db_on  & 0x1) << 1;
		cpclcan.can.cd.us[2] |= (gevcufunction.psw[PSW_PB_ARM]->db_on   & 0x1) << 2;
		cpclcan.can.cd.us[2] |= (gevcufunction.psw[PSW_ZODOMTR]->db_on  & 0x1) << 3;
		cpclcan.can.cd.us[2] |= (gevcufunction.psw[PSW_ZTENSION]->db_on & 0x1) << 4;
		cpclcan.can.cd.us[2] |= (psw_cl_fs_no->db_on  & 0x1) << 5;
		cpclcan.can.cd.us[2] |= (psw_cl_rst_n0->db_on & 0x1) << 6;

		/* pay[6:7] CL position uint16_t 0-10000 */
		cpclcan.can.cd.us[3] = (clfunc.curpos * 100);
		xQueueSendToBack(CanTxQHandle,&cpclcan,4);
	}
}