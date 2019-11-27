/******************************************************************************
* File Name          : calib_control_lever.h
* Date First Issued  : 10/12/2019, hack of 08/31/2014
* Board              : DiscoveryF4
* Description        : Master Controller: Control Lever calibration
*******************************************************************************/
#ifndef __CALIB_CONTROL_LEVER
#define __CALIB_CONTROL_LEVER

#include <stdio.h>

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
	uint8_t state;   // Calibration state; 
};

/* *********************************************************************************************************** */
void calib_control_lever_init();
/* @brief	: Prep for CL calibration
 ************************************************************************************************************* */
void calib_control_lever(void);
/* @brief	: Calibrate CL
 ************************************************************************************************************* */

#endif 

