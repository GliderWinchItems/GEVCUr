/******************************************************************************
* File Name          : lcdprintf.c
* Date First Issued  : 10/01/2019
* Description        : LCD display printf
*******************************************************************************/
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "lcdprintf.h"
#include "queue.h"
#include "malloc.h"
#include "stm32f4xx_hal_usart.h"
#include "stm32f4xx_hal_uart.h"
#include "4x20lcd.h"
#include "yprintf.h"

/*	LCD Line Size  */
#define LCDLINESIZE 20
#define LCDROWSIZE 4
#define LCD_BACKLIGHT_LEVEL 70

/* **************************************************************************************
 * void lcdprintf_init(struct SERIALSENDTASKBCB** ppbcb);
 * @brief	: Initialize display
 * @param	: ppbcb = pointer to pointer to serial buffer control block
 * ************************************************************************************** */
void lcdprintf_init(struct SERIALSENDTASKBCB** ppbcb)
{
	struct SERIALSENDTASKBCB* pbcb = *ppbcb;

	yprintf_init();	// JIC not init'd

	/*	wait a half second for LCD to finish splash screen. */
//	osDelay(pdMS_TO_TICKS(500)); // This is done in GevcuTask/calib_control_lever

	/* Build line of control chars to init display. */
	uint8_t* p = (uint8_t*)lcd_init(pbcb->pbuf);
	*p = 0; // Add string terminator

	pbcb->size = (p - pbcb->pbuf); // (Is this needed?)

	/* Place Buffer Control Block on queue to SerialTaskSend */
	vSerialTaskSendQueueBuf(ppbcb); // Place on queue

	return;
}
/* **************************************************************************************
int lcdprintf(struct SERIALSENDTASKBCB** ppbcb, int row, int col, const char *fmt, ...);
 * @brief	: 'printf' for uarts
 * @param	: pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
 * @param	: row = row (line) number (0-3)
 * @param	: col = column number (0-19)
 * @param	: format = usual printf format
 * @param	: ... = usual printf arguments
 * @return	: Number of chars "printed"
 * ************************************************************************************** */
uint32_t lcddbg;

int lcdprintf(struct SERIALSENDTASKBCB** ppbcb, int row, int col, const char *fmt, ...)
{
	struct SERIALSENDTASKBCB* pbcb = *ppbcb;
	va_list argp;

	/* Block if this buffer is not available. SerialSendTask will 'give' the semaphore 
      when the buffer has been sent. */
	xSemaphoreTake(pbcb->semaphore, 6001);

	/* Block if vsnprintf is being uses by someone else. */
	xSemaphoreTake( vsnprintfSemaphoreHandle, portMAX_DELAY );

	/* Construct line of data.  Stop filling buffer if it is full. */
	va_start(argp, fmt);
	pbcb->size = vsnprintf((char*)(pbcb->pbuf+2),pbcb->maxsize, fmt, argp);
	va_end(argp);

	/* Line to send has two leading control/command bytes. */
	pbcb->size += 2; // Adjust size

	/* Limit byte count in BCB to be put on queue, from vsnprintf to max buffer sizes. */
	if (pbcb->size > pbcb->maxsize) 
			pbcb->size = pbcb->maxsize;

	/* Set row & column codes */
	uint8_t* p = pbcb->pbuf;
	*p++ = (254); // move cursor command

	// determine position
	if (row == 0) {
		*p = (128 + col);
	} else if (row == 1) {
		*p = (192 + col);
	} else if (row == 2) {
		*p = (148 + col);
	} else if (row == 3) {
		*p = (212 + col);
	}

	/* Release semaphore controlling vsnprintf. */
	xSemaphoreGive( vsnprintfSemaphoreHandle );

	/* JIC */
	if (pbcb->size == 0) return 0;

	/* Place Buffer Control Block on queue to SerialTaskSend */
	vSerialTaskSendQueueBuf(ppbcb); // Place on queue

	return pbcb->size;
}
/* **************************************************************************************
 * int lcdputs(struct SERIALSENDTASKBCB** ppbcb, char* pchr);
 * @brief	: Send zero terminated string to SerialTaskSend
 * @param	: pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
 * @param	: pchr = pointer to input string to 'put'
 * @return	: Number of chars sent
 * ************************************************************************************** */
int lcdputs(struct SERIALSENDTASKBCB** ppbcb, char* pchr)
{
	struct SERIALSENDTASKBCB* pbcb = *ppbcb;
	int sz = strlen(pchr); // Check length of input string
	if (sz == 0) return 0;

	/* Block if this buffer is not available. SerialSendTask will 'give' the semaphore 
      when the buffer has been sent. */
	xSemaphoreTake(pbcb->semaphore, 6002);

	strncpy((char*)pbcb->pbuf,pchr,pbcb->maxsize);	// Copy and limit size.

	/* Set size serial send will use. */
	if (sz >= pbcb->maxsize)	// Did strcpy truncate?
		pbcb->size = pbcb->maxsize;	// Yes
	else
		pbcb->size = sz;	// No

	vSerialTaskSendQueueBuf(ppbcb); // Place on queue
	return pbcb->size; 
}


