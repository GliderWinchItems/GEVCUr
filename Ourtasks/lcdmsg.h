/******************************************************************************
* File Name          : lcdmsg.h
* Date First Issued  : 02/29/2020
* Description        : Conslidate LCD printf'ing
*******************************************************************************/

#ifndef __LCDMSG
#define __LCDMSG

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* *************************************************************************/
osMessageQId lcdmsg_init(uint16_t qsize);
/*	@brief	: Setup the queue for pointers
 * @return	: NULL = failed; pointer to queue handle
 * *************************************************************************/
void lcdmsg_poll(void);
/*	@brief	: 
 * *************************************************************************/

extern osMessageQId lcdmsgQHandle;

#endif

