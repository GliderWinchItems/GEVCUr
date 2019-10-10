/******************************************************************************
* File Name          : adcfastsum16.h
* Date First Issued  : 03/16/2019
* Description        : Fast sum for summimng ADC DMA buffering
*******************************************************************************/

#ifndef __ADCFASTSUM16
#define __ADCFASTSUM16

#include <stdint.h>

#define ADCFASTSUM16SIZE 16 // Number of seqs hard-coded. Use for checks

/* *************************************************************************/
void adcfastsum16(uint16_t* psum, uint16_t* pdma);
/*	@brief	: Inline fast summation: ASSUMES 16 ADC sequences: channels: ADC1IDX_ADCSCANSIZE
 * @param	: psum = pointer to sum
 * @param	: pdma = pointer to dma buffer
 * *************************************************************************/

#endif
