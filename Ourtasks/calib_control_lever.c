/******************************************************************************
* File Name          : calib_control_lever.c
* Date First Issued  : 08/31/2014
* Board              : DiscoveryF4
* Description        : Master Controller: Control Lever calibration
*******************************************************************************/

#include "calib_control_lever.h"
#include "etmc0.h"
#include "mc_msgs.h"
#include "adc_mc.h"
#include "libopencm3/stm32/f4/gpio.h"
#include "bsp_uart.h"
#include "init_hardware_mc.h"
#include "clockspecifysetup.h"
#include "xprintf.h"
#include <string.h>
#include <stdio.h>
#include "spi2rw.h"
#include "4x20lcd.h"
#include "beep_n_lcd.h"




/* ***********************************************************************************************************
 * void calib_control_lever(void);
 * @brief	:Setup & initialization functions 
 ************************************************************************************************************* */

//	code for calibrating scale and offset for the control lever
//	make function later
//#define FSCL	((1 << 12) - 1)	// full scale control lever (CL) output
#define CLREST (1 << 11) 	// SPI bit position for CL rest position switch
#define CLFS  (1 << 8) 		// SPI bit position for CL full scale position
#define CL_ADC_CHANNEL 	0
#define FLASHCOUNT (sysclk_freq/8);	// Orange LED flash increment

// Min and maximum values observed for control lever
static int cloffset = 0; 
static int clmax = 0;	
static float fpclscale;		//	CL conversion scale factor 

void calib_control_lever(struct ETMCVAR* petmcvar)
{
	int clcalstate = 0;		// state for control lever intial calibration
	int i;					// loop counter
	int adc_tmp;
	unsigned int t_led = DTWTIME + FLASHCOUNT;	//	initial t_led
	char vv[128];

	int ledCount = 0;
	int cycleCount = 1;
	#define ledPatternLength 32
	#define ledLag 3
	//	led test pattern with extension to effect circular behavior
		u32 ledTestPattern[] = 
	{
		0xffff,
		0xffff,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0xffff,
		0xffff,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		LED_SAFE,
		LED_PREP,
		LED_ARM,
		LED_STOP,
		LED_GNDRLRTN,
		LED_RAMP,
		LED_CLIMB,
		LED_ABORT,
		LED_RECOVERY,
		LED_PREP,
		LED_RETRIEVE,
		LED_SAFE,		
		LED_ARM_PB,
		LED_PREP_PB,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0x0000,
		0xffff,
		0xffff,
		0x0000,
		0x0000,
		0x0000,
		0x0000
	};

	if (GPIOB_IDR & (1 << 1)) //	test for physical or glass CP
	{	//	physical CP
		xprintf (UXPRT,"\nBegin control lever calibration\n\r");
		// dummy read of SPI switches to deal with false 0000 initially returned
		spi2_rw(petmcvar->spi_ledout, petmcvar->spi_swin, SPI2SIZE);
		while(clcalstate < 6)
		{
			if (((int)(DTWTIME - t_led)) > 0) // Has the time expired?
			{ //	Time expired
				//	read filtered control lever adc last value and update min and max values
				adc_tmp = adc_last_filtered[CL_ADC_CHANNEL];
				cloffset = (cloffset < adc_tmp) ? cloffset : adc_tmp;
				clmax = (clmax > adc_tmp) ? clmax : adc_tmp;
				//	Read SPI switches
				//	get most current switch positions
				
				//	Not sure why 
				/*if (spi2_busy() != 0) // Is SPI2 busy?
				{ // SPI completed  
					spi2_rw(petmcvar->spi_ledout, petmcvar->spi_swin, SPI2SIZE); // Send/rcv SPI2SIZE bytes
					//	convert to a binary word for comparisons (not general for different SPI2SIZE)
					sw = (((int) petmcvar->spi_swin[0]) << 8) | (int) petmcvar->spi_swin[1];
					//sw = petmcvar->spi_swin;	*/
				
				//	usage of spi2rw() is bad.  Should wait for not busy after starting it.

				if (spi2_busy() != 0) // Is SPI2 busy?
				{ // Here, no.
					u32 tmp = petmcvar->cp_outputs;
	    			for (i = SPI2SIZE - 1; i >= 0; i--)
	    			{
	    				petmcvar->spi_ledout[i] = (char) tmp;
	    				tmp >>= 8;
	    			}
	    			petmcvar->cp_inputs = (((int) petmcvar->spi_swin[0]) << 8) | (int) petmcvar->spi_swin[1];
					spi2_rw(petmcvar->spi_ledout, petmcvar->spi_swin, SPI2SIZE); 
					xprintf(UXPRT, "%5u %5d %8x \n\r", clcalstate, adc_tmp, petmcvar->cp_inputs);	

					petmcvar->cp_outputs = 0;
					if ((clcalstate == 1) || (clcalstate == 2))
					{
						// LEDs chasing their tails
						for (i = 0; i < ledLag; i++)
						{
							petmcvar->cp_outputs |= ledTestPattern[ledCount + i];
						}
					}

					switch(clcalstate)
					{				
						case 0:	//	entry state
						{
							sprintf(vv, "Cycle control lever");
							lcd_printToLine(UARTLCD, 0, vv);
							sprintf(vv, "twice:");
							lcd_printToLine(UARTLCD, 1, vv);
							double_beep();
							clcalstate = 1;
						}
						case 1:	// waiting for CL to rest position	
						{
							if (petmcvar->cp_inputs & CLREST) break;
							clcalstate = 2;
							cloffset = clmax = adc_tmp;	//	reset min and max values
							sprintf(vv, "twice: 0");
							lcd_printToLine(UARTLCD, 1, vv);
							break;
						}
						case 2:	//	waiting for full scale position first time
						{
							if (petmcvar->cp_inputs & CLFS) break ;
							clcalstate = 3;
							sprintf(vv, "twice: 0.5");
							lcd_printToLine(UARTLCD, 1, vv);
							break;						
						}
						case 3:	//	wating for return to rest first time
						{
							if (petmcvar->cp_inputs & CLREST) break; 
							clcalstate = 4;
							// clcalstate = 6;		//	only requires 1 cycle
							sprintf(vv, "twice: 1  ");
							lcd_printToLine(UARTLCD, 1, vv);
							single_beep();
							break;
						}
						case 4:	//	waiting for full scale second time
						{
							if (petmcvar->cp_inputs & CLFS) break;
							clcalstate = 5;
							sprintf(vv, "twice: 1.5");
							lcd_printToLine(UARTLCD, 1, vv);
							break;					
						}
						case 5:	//	waiting for return to rest second time
						{
							if (petmcvar->cp_inputs & CLREST) break;
							single_beep();
							clcalstate = 6; 
						}
					}			
				}
				toggle_4leds(); 	// Advance some LED pattern
				ledCount++;
				if (ledCount >= ledPatternLength)
				{
					ledCount = 0;
				} 
				t_led += FLASHCOUNT; 	// Set next toggle time	
			}
		}	
		fpclscale = 1.0 / (clmax - cloffset);
		lcd_clear(UARTLCD);
		xprintf(UXPRT, "  cloffset: %10d clmax: %10d\n\r", cloffset, clmax);
		xprintf 	(UXPRT, "   Control Lever Initial Calibration Complete\n\r");
	}
	else
	{	
		xprintf (UXPRT,"\nGlass CP indicated\n\r");
		spi2_rw(petmcvar->spi_ledout, petmcvar->spi_swin, SPI2SIZE);
		while(clcalstate < 2)
		{
			if (((int)(DTWTIME - t_led)) > 0) // Has the time expired?
			{ //	Time expired
				if (spi2_busy() != 0) // Is SPI2 busy?
				{ // Here, no.
					u32 tmp = petmcvar->cp_outputs;
	    			for (i = SPI2SIZE - 1; i >= 0; i--)
	    			{
	    				petmcvar->spi_ledout[i] = (char) tmp;
	    				tmp >>= 8;
	    			}
	    			petmcvar->cp_inputs = (((int) petmcvar->spi_swin[0]) << 8) | (int) petmcvar->spi_swin[1];
					spi2_rw(petmcvar->spi_ledout, petmcvar->spi_swin, SPI2SIZE); 
					petmcvar->cp_outputs = 0;					
					// CP LEDs chasing their tails
					for (i = 0; i < ledLag; i++)
					{
						petmcvar->cp_outputs |= ledTestPattern[ledCount + i];
					}								
				}
				switch(clcalstate)
					{				
						case 0:	//	entry state
						{
							sprintf(vv, "Glass CP Indicated");
							lcd_printToLine(UARTLCD, 0, vv);
							double_beep();
							clcalstate = 1;
						}
						case 1:	//	flash the lights through several cycles
						{
							if (cycleCount <= 0)
							{
								single_beep();
								clcalstate = 2;
							}

						}
					}
				toggle_4leds(); 	// Advance some LED pattern
				ledCount++;
				if (ledCount >= ledPatternLength)
				{
					ledCount = 0;
					cycleCount--;
				} 
				t_led += FLASHCOUNT; 	// Set next toggle time	
			}
		}
		lcd_clear(UARTLCD);
	}
	return;
}
/* ***********************************************************************************************************
 * int calib_control_lever_get(void);
 * @brief	:
 * @return	: Calibrated Control lever: 0 - 4095 (but could be slightly negative) -> 0 - 100%
 ************************************************************************************************************* */
float calib_control_lever_get(void)
{
	float x;
	x = (adc_last_filtered[CL_ADC_CHANNEL] - cloffset) * fpclscale;
	/* for now do not limit
	x = x > 1.0 ? 1.0 : x;
	x = x < 0.0 ? 0.0 : x;
	*/
	return x;
}


