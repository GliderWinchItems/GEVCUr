/******************************************************************************
* File Name          : control_law_v0.h
* Date First Issued  : 03/19/2020
* Board              : DiscoveryF4
* Description        : Compute torque request for dmoc
*******************************************************************************/
#ifndef __CONTROL_LAW_V0
#define __CONTROL_LAW_V0

#include "dmoc_control.h"

/* *************************************************************************/
void control_law_v0_calc(struct DMOCCTL* pdmocctl);
/* @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/


#endif
