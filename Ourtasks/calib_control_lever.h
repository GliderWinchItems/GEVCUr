/******************************************************************************
* File Name          : calib_control_lever.h
* Date First Issued  : 01/26/2020
* Board              : DiscoveryF4
* Description        : Master Controller: Control Lever calibration
*******************************************************************************/
#ifndef __CALIB_CONTROL_LEVER
#define __CALIB_CONTROL_LEVER

#include <stdio.h>

enum CLSTATE
{
	INITLCD,
	INITLCD1,
	INITLCD2,
	INITLCD3,
	INITLCD4,
	INITLCD5,
	INITLCD6,
	CLOSE1,
	CLOSE1WAIT,
	OPEN1,
	OPEN1WAIT,
	CLOSE2,
	CLOSE2WAIT,
	CLCREADY,   // CL calibration complete
	SEQDONE
};

struct CLFUNCTION
{
// Min and maximum values observed for control lever
	float min;       // Offset
	float max;       // Reading for max
	float minends;   // min + deadzone rest postiion
	float maxbegins; // max - deadzone full position
	float rcp_range; // (reciprocal) 100.0/(maxbegins - minends)
	float curpos;    // Current position (pct)
	/* Control Lever full close and full open zones. */
	float deadr;     // Dead zone for rest position (pct)
	float deadf;     // Dead zone for full position (pct) 
	uint32_t timx;	  // GevcuTask timer tick for next state
	uint16_t toctr;  // Prompt timeOut counter
	uint8_t state;   // Calibration state; 
};

/* *********************************************************************************************************** */
void calib_control_lever_init();
/* @brief	: Prep for CL calibration
 ************************************************************************************************************* */
float calib_control_lever(void);
/* @brief	: Calibrate CL
 * @param	: control lever postion as a percent (0 - 100)
 * @return	: Return lever position as percent
 ************************************************************************************************************* */

extern struct CLFUNCTION clfunc;
extern uint8_t flag_clcalibed; 

#endif 

