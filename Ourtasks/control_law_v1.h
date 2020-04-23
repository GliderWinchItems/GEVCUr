/******************************************************************************
* File Name          : control_law_v1.h
* Date First Issued  : 03/22/2020
* Board              : DiscoveryF4
* Description        : Compute torque request for dmoc--PI Loop
*******************************************************************************/
#ifndef __CONTROL_LAW_V1
#define __CONTROL_LAW_V1

#include "dmoc_control.h"

struct CTLLAWPILOOP // Control Law PI Loop
{
	//	Working variables
	float spderr;	//	speed error
	float dsrdspd;	//	desired speed
	float intgrtr;	//	PI integrator

	//	Parameters
	float kp;    	// Proportional constant
	float ki;    	// Integral constant
	float clpi;		//	integrator anti-windup clip level
	float clpcp;	//	command clip level positive
	float clpcn;	//	command clip level negative
	float fllspd;	//	100% control lever speed magnitude
};

/* *************************************************************************/
void control_law_v1_init(void);
/* @brief	: Load parameters
 * *************************************************************************/
 void control_law_v1_reset(void);
/* @brief	: Reset
 * *************************************************************************/
void control_law_v1_calc(struct DMOCCTL* pdmocctl);
/* @param	: pdmocctl = pointer to struct with "everything" for this DMOC unit
 * @brief	: Compute torquereq
 * *************************************************************************/

extern struct CTLLAWPILOOP clv1;

#endif

