/******************************************************************************
* File Name          : GevcuStates.c
* Date First Issued  : 07/01/2019
* Description        : States in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"

#include "GevcuStates.h"
#include "GevcuTask.h"
#include "gevcu_idx_v_struct.h"
#include "morse.h"
#include "adcparamsinit.h"



