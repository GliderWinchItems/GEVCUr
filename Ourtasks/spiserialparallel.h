/******************************************************************************
* File Name          : spiserialparallel.h
* Date First Issued  : 10/13/2019
* Description        : SPI serial<->parallel extension
*******************************************************************************/

#ifndef __SPISERIALPARALLEL
#define __SPISERIALPARALLEL

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"

#define SPISERIALPARALLELSIZE 2 // Number of bytes write/read

union SPISP
{
	uint16_t u16;
	uint8_t  u8[SPISERIALPARALLELSIZE];
};

/* *************************************************************************/
HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi);
/*	@brief	: 
 * @return	: 0 = success; not 0 = fail
 * *************************************************************************/

extern union SPISP spisp_rd[SPISERIALPARALLELSIZE/2];
extern union SPISP spisp_wr[SPISERIALPARALLELSIZE/2];
extern uint32_t spispctr; // Cycle counter


#endif

