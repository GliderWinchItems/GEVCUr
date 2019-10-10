/******************************************************************************
* File Name          : beep_n_lcd.h
* Date First Issued  : 08/31/2014,10/01/2019
* Board              : DiscoveryF4
* Description        : Master Controller: simple beeping, lcd alert routines
*******************************************************************************/

#ifndef __BEEP_N_LCD
#define __BEEP_N_LCD

#include "common_misc.h"
#include "common_can.h"
#include "etmc0.h"




void beep_n(int n, struct ETMCVAR* petmcvar);	//	n non-blocking beeps
void beep_poll(struct ETMCVAR* petmcvar);		//	beep generator
/* *********************************************************** */
void show_op_the_error(char* p, int e, unsigned int t);
/* @brief	: Show Op the error, beep, and pause
 * @param	: p = pointer to string that goes on line 0 of LCD
 * @param	: e = error code
 * @param	: t = number of 1/10th secs to pause before continuing.
************************************************************** */
void toggle_4leds (void);
/* @brief	: Turn the LEDs on in sequence, then turn them back off 
************************************************************** */
void toggle_led (int lnum);
/* @brief	: Toggle one LED
  * @param	: lnum = led number (pin number)
************************************************************** */




#endif 

