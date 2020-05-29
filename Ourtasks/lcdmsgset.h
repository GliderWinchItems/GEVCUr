/******************************************************************************
* File Name          : lcdmsg.h
* Date First Issued  : 02/29/2020
* Description        : Conslidate LCD printf'ing
*******************************************************************************/

#ifndef __LCDMSGSET
#define __LCDMSGSET

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/* *************************************************************************/
osMessageQId lcdmsgset_init(uint16_t qsize);
/*	@brief	: Setup the queue for pointers
 * @return	: NULL = failed; pointer to queue handle
 * *************************************************************************/
void lcdmsgset_poll(void);
/*	@brief	: 
 * *************************************************************************/

extern osMessageQId lcdmsgsetQHandle;

#endif
