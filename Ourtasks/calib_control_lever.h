/******************************************************************************
* File Name          : calib_control_lever.h
* Date First Issued  : 10/12/2019, hack of 08/31/2014
* Board              : DiscoveryF4
* Description        : Master Controller: Control Lever calibration
*******************************************************************************/


#ifndef __CALIB_CONTROL_LEVER
#define __CALIB_CONTROL_LEVER

#include "common_misc.h"
#include "common_can.h"
#include "adcparams.h"

#define SPI2SIZE	2

// SPI_LED assignements (these should become mc system parameters in rewrite)
#define LED_SAFE        0x8000
#define LED_PREP        0x4000
#define LED_ARM         0x2000
#define LED_GNDRLRTN    0x1000
#define LED_RAMP        0x0800
#define LED_CLIMB       0x0400
#define LED_RECOVERY    0x0200
#define LED_RETRIEVE    0x0100
#define LED_STOP        0x0080
#define LED_ABORT       0x0040
#define LED_PREP_PB     0x0002
#define LED_ARM_PB      0x0001


//	control panel switch mapping
#define SW_SAFE   1 << 15	//	active low
#define SW_ACTIVE 1 << 14	//	active low
#define PB_ARM    1 << 13	//	active low
#define PB_PREP   1 << 12	//	active low
#define CL_RST_N0 1 << 11	//	low at rest
#define CL_FS_NO  1 << 8	// 	low at full scale

#define CP_OUTPUTS_HB_COUNT 64
#define CP_INPUTS_HB_COUNT 	48
#define CP_CL_HB_COUNT 		24
#define CP_CL_DELTA			0.005
#define CP_LCD_HB_COUNT 	16

struct CLFUNCTION
{
// Min and maximum values observed for control lever
	int cloffset; 
	int clmax;	
	float fpclscale;		//	CL conversion scale factor 	


};

/* *********************************************************************************************************** */
void calib_control_lever(struct ETMCVAR* petmcvar);
/* @brief	:Setup & initialization functions 
 ************************************************************************************************************* */
float calib_control_lever_get(void);
/* @brief	:
 * @return	: Calibrated control lever output between 0.0 and 1.0
 ************************************************************************************************************* */

#endif 

