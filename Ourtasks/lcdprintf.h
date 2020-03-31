/******************************************************************************
* File Name          : lcdprintf.h
* Date First Issued  : 10/01/2019
* Description        : LCD display printf
*******************************************************************************/

#ifndef __LCDPRINTF
#define __LCDPRINTF

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "SerialTaskSend.h"

/* LCD row assigments.  */
#define GEVCUTSK  0 // GevcuStates: state msg
#define CLROW     1 // calib_control_lever: calibration & position
#define CNCTRLCDA 2 // contactor_control_msg: state msg
#define CNCTRLCDB 3 // contactor_control_msg: fault msg
#define DMOCSPDTQ 3 // GevcuStates: speed:torque


/* *************************************************************************/
void lcdprintf_init(struct SERIALSENDTASKBCB** ppbcb);
/* @brief	: Initialize display
 * @param	: ppbcb = pointer to pointer to serial buffer control block
 * *************************************************************************/
int lcdprintf(struct SERIALSENDTASKBCB** ppbcb, int row, int col, const char *fmt, ...);
/* @brief	: 'printf' for uarts
 * @param	: pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
 * @param	: row = row (line) number (0-3)
 * @param	: col = column number (0-19)
 * @param	: format = usual printf format
 * @param	: ... = usual printf arguments
 * @return	: Number of chars "printed"
 * ************************************************************************************** */
int lcdputs(struct SERIALSENDTASKBCB** ppbcb, char* pchr);
/* @brief	: Send zero terminated string to SerialTaskSend
 * @param	: pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
 * @param	: pchr = pointer to input string to 'put'
 * @return	: Number of chars sent
 * ************************************************************************************** */

#endif

