/******************************************************************************
* File Name          : control_law_v1.h
* Date First Issued  : 03/22/2020
* Board              : DiscoveryF4
* Description        : Compute torque request for dmoc--PI Loop
*******************************************************************************/
#ifndef __CONTROL_LAW_V1
#define __CONTROL_LAW_V1

#include "dmoc_control.h"


/* *************************************************************************/
void control_law_v1_init(void);
/* @brief	: Load parameters
 * *************************************************************************/
void control_law_v1_calc(struct DMOCCTL* pdmocctl);
/* @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/


#endif
