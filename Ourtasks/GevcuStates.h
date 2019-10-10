/******************************************************************************
* File Name          : GevcuStates.h
* Date First Issued  : 07/01/2019
* Description        : States in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#ifndef __GEVCUSTATES
#define __GEVCUSTATES

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "adc_idx_v_struct.h"


void GevcuStates_otosettling_init(struct GEVCUFUNCTION* pcf);
void GevcuStates_disconnecting(struct GEVCUFUNCTION* pcf);
void GevcuStates_disconnected(struct GEVCUFUNCTION* pcf);
void GevcuStates_connecting(struct GEVCUFUNCTION* pcf);
void GevcuStates_connected(struct GEVCUFUNCTION* pcf);
void GevcuStates_faulting(struct GEVCUFUNCTION* pcf);
void GevcuStates_faulted(struct GEVCUFUNCTION* pcf);
void GevcuStates_reset(struct GEVCUFUNCTION* pcf);
void transition_faulting(struct GEVCUFUNCTION* pcf, uint8_t fc);

#endif

