/******************************************************************************
* File Name          : calib_control_lever.h
* Date First Issued  : 08/31/2014
* Board              : DiscoveryF4
* Description        : Master Controller: Control Lever calibration
*******************************************************************************/


#ifndef __CALIB_CONTROL_LEVER
#define __CALIB_CONTROL_LEVER

#include "common_misc.h"
#include "common_can.h"
#include "etmc0.h"



/* *********************************************************************************************************** */
void calib_control_lever(struct ETMCVAR* petmcvar);
/* @brief	:Setup & initialization functions 
 ************************************************************************************************************* */
float calib_control_lever_get(void);
/* @brief	:
 * @return	: Calibrated control lever output between 0.0 and 1.0
 ************************************************************************************************************* */

#endif 

