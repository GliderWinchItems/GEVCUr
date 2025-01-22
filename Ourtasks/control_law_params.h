/******************************************************************************
* File Name          : control_law_params.h
* Date First Issued  : 01/22/2025
* Description        : Parameters for control_law routines and tests
* Usage              : 
*******************************************************************************/
#ifndef __CONTROL_LAW_PARAMS
#define __CONTROL_LAW_PARAMS

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
	float clpc;		//	command clip level
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

