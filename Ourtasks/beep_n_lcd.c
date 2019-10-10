/******************************************************************************
* File Name          : beep_n_lcd.c
* Date First Issued  : 08/31/2014
* Board              : DiscoveryF4
* Description        : Master Controller: simple beeping, lcd alert routines
*******************************************************************************/

#include "beep_n_lcd.h"
#include "mc_msgs.h"
#include "bsp_uart.h"
#include "4x20lcd.h"
#include "4x20lcd.h"
#include "xprintf.h"
#include <string.h>
#include <stdio.h>
#include "clockspecifysetup.h"
#include "mc_msgs.h"
#include "libopencm3/stm32/f4/gpio.h"
#include "init_hardware_mc.h"
#include "etmc0.h"




/* LED identification
Discovery F4 LEDs: PD 12, 13, 14, 15

12 green   
13 orange
14 red
15 blue
*/

/* ***********************************************************
 * void delay_tenth_sec(unsigned int t);
 * @brief	: Looping delay using DTW counter
 * @param	: t = number of 1/10th secs to delay
***************************************************************/
void delay_tenth_sec(unsigned int t)
{
	int i;
	unsigned int tp = sysclk_freq/10;	// Increment for 1/10th sec
	volatile unsigned int t0 = DTWTIME + tp;
	for (i = 0; i < t; i++)
	{
		while (((int)(DTWTIME - t0)) < 0); // Has the time expired?
		t0 += tp;
	}
	return;
}
/* ************************************************************
 * void single_beep(void);	// One beep
 * void double_beep(void);	// Two beeps
 * void triple_beep(void);	// Three beeps
***************************************************************/
//Old blocking beeper functionsfunctions


void single_beep(void)
{
	GPIO_BSRR(GPIOA) = 1 << 8;	// Turn on beeper
	delay_tenth_sec(1);		// 1/10th sec ON
	GPIO_BSRR(GPIOA) = 1 << (8 + 16);// Turn off beeper
	delay_tenth_sec(2);		// 1/10th sec OFF
	return;	
}
void double_beep(void)
{
	single_beep();
	single_beep();
}
void triple_beep(void)
{
	single_beep();
	single_beep();
	single_beep();
}


//	non-blocking beep funtions
void beep_n(int n, struct ETMCVAR* petmcvar)
{
	petmcvar->beep_count += n;
	return;
}

void beep_poll(struct ETMCVAR* petmcvar)
{
/*	beep_state values
*	idle	0
*	beeping	1
*	delay 	2
*/
	switch(petmcvar->beep_state)
	{
		case 0:	//	idle
		{
			if (petmcvar->beep_count != 0)
			{
				petmcvar->beep_time = DTWTIME + sysclk_freq/10;	//	beep for 1/10 second beep
				GPIO_BSRR(GPIOA) = 1 << 8;	// Turn on beeper
				petmcvar->beep_state = 1;
			}				
			break;
		}
		case 1:	//	beeping
		{
			if (((int)(DTWTIME - petmcvar->beep_time) > 0))
			{
				petmcvar->beep_time = DTWTIME + sysclk_freq/5;	//	delay for 1/5 second
				GPIO_BSRR(GPIOA) = 1 << (8 + 16);	// Turn off beeper
				petmcvar->beep_state = 2;
			}
			break;
		}
		case 2:	//	delay following beep;
		{
			if (((int)(DTWTIME - petmcvar->beep_time) > 0))
			{
				(petmcvar->beep_count)--;	//	if 0 will start next beep on next entry
				petmcvar->beep_state = 0;
			}
		}
		return;
	}
}

/* ************************************************************
 * void show_op_the_error(char* p, int e, unsigned int t);
 * @brief	: Show Op the error, beep, and pause
 * @param	: p = pointer to string that goes on line 0 of LCD
 * @param	: e = error code
 * @param	: t = number of 1/10th secs to pause before continuing.
***************************************************************/
void show_op_the_error(char* p, int e, unsigned int t)
{
	char vv[96];
	lcd_printToLine(UARTLCD,0, p);	// Line 0 tells "who"
	sprintf(vv, "er code: %d",e); 	
	lcd_printToLine(UARTLCD, 1, vv);	// Line 1 shows error code
	xprintf(UXPRT,"ERROR: %s er code: %s\n\r",p, vv); // Output to debugging serial port
	triple_beep();
	delay_tenth_sec(t);
	return;
}
/* ************************************************************
 * void toggle_4leds (void);
 * @brief	: Turn the LEDs on in sequence, then turn them back off 
***************************************************************/
static int lednum = 12;	// Lowest port bit numbered LED
void toggle_4leds (void)
{
	if ((GPIO_ODR(GPIOD) & (1<<lednum)) == 0)
	{ // Here, LED bit was off
		GPIO_BSRR(GPIOD) = (1<<lednum);	// Set bit
	}
	else
	{ // Here, LED bit was on
		GPIO_BSRR(GPIOD) = (1<<(lednum+16));	// Reset bit
	}
	lednum += 1;		// Step through all four LEDs
	if (lednum > 15) lednum = 12;

}


/* ************************************************************
  * void toggle_led (int lnum);
  * @brief	: Toggle one LED
  * @param	: lnum = led number (pin number)
***************************************************************/
void toggle_led (int lnum)
{
	if ((GPIO_ODR(GPIOD) & (1<<lnum)) == 0)
	{ // Here, LED bit was off
		GPIO_BSRR(GPIOD) = (1<<lnum);	// Set bit
	}
	else
	{ // Here, LED bit was on
		GPIO_BSRR(GPIOD) = (1<<(lnum+16));	// Reset bit
	}

}




