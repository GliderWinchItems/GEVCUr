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
#include "SwitchTask.h"

void GevcuEvents_00(void);
void GevcuEvents_01(void);
void GevcuEvents_02(struct SWITCHPTR* psw); //psw = pointer to switch struct
void GevcuEvents_03(struct SWITCHPTR* psw); //psw = pointer to switch struct
void GevcuEvents_04(void);
void GevcuEvents_05(void);
void GevcuEvents_06(void);
void GevcuEvents_07(void);
void GevcuEvents_08(void);
void GevcuEvents_09(void);
void GevcuEvents_10(void);
void GevcuEvents_11(void);
void GevcuEvents_12(void);
void GevcuEvents_13(void);
void GevcuEvents_14(void);
void GevcuEvents_15(void);

#endif

