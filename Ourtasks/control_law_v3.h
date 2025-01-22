/******************************************************************************
* File Name          : control_law_v3.h
* Date First Issued  : 01/20/2025
* Board              : DiscoveryF4
* Description        : Control for auto inertia testing
*******************************************************************************/
#ifndef __CONTROL_LAW_V3
#define __CONTROL_LAW_V3

#include "dmoc_control.h"

/* *************************************************************************/
void control_law_v3_init(void);
/* @brief	: Load parameters
 * *************************************************************************/
void control_law_v3_calc(struct DMOCCTL* pdmocctl);
/* @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/

#endif
