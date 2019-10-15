/******************************************************************************
* File Name          : GevcuEvents.h
* Date First Issued  : 07/01/2019
* Description        : Events in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#ifndef __GEVCUEVENTS
#define __GEVCUEVENTS

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "adc_idx_v_struct.h"

void GevcuEvents_00(struct GEVCUFUNCTION* pcf);
void GevcuEvents_01(struct GEVCUFUNCTION* pcf);
void GevcuEvents_02(struct GEVCUFUNCTION* pcf);
void GevcuEvents_03(struct GEVCUFUNCTION* pcf);
void GevcuEvents_04(struct GEVCUFUNCTION* pcf);
void GevcuEvents_05(struct GEVCUFUNCTION* pcf);
void GevcuEvents_06(struct GEVCUFUNCTION* pcf);
void GevcuEvents_07(struct GEVCUFUNCTION* pcf);
void GevcuEvents_08(struct GEVCUFUNCTION* pcf);
void GevcuEvents_09(struct GEVCUFUNCTION* pcf);
void GevcuEvents_10(struct GEVCUFUNCTION* pcf);
void GevcuEvents_11(struct GEVCUFUNCTION* pcf);
void GevcuEvents_12(struct GEVCUFUNCTION* pcf);
void GevcuEvents_13(struct GEVCUFUNCTION* pcf);
void GevcuEvents_14(struct GEVCUFUNCTION* pcf);

#endif

