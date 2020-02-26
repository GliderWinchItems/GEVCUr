/******************************************************************************
* File Name          : contactor_control_msg.h
* Date First Issued  : 02/25/2020
* Board              : DiscoveryF4
* Description        : Status msgs received from contactor unit
*******************************************************************************/

#ifndef __CONTACTOR_CONTROL_MSG
#define __CONTACTOR_CONTROL_MSG

/******************************************************************************/
void contactor_control_msg(struct CANRCVBUF* p);
/* @brief 	: Send CAN msgs
 * @param	: p = pointer to CAN msg response from contactor
*******************************************************************************/

#endif

