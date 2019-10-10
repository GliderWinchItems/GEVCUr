/******************************************************************************
* File Name          : lcdprintf.c
* Date First Issued  : 10/01/2019
* Description        : LCD display printf
*******************************************************************************/
#include <stdarg.h>
#include "lcdprintf.h"
#include "queue.h"
#include "malloc.h"
#include "stm32f4xx_hal_usart.h"
#include "stm32f4xx_hal_uart.h"
#include "4x20lcd.h"
#include "yprintf.h"

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
	osDelay(pdMS_TO_TICKS(500));		

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
int lcdprintf(struct SERIALSENDTASKBCB** ppbcb, int row, int col, const char *fmt, ...)
{
	struct SERIALSENDTASKBCB* pbcb = *ppbcb;
	va_list argp;

	/* Block if this buffer is not available. SerialSendTask will 'give' the semaphore 
      when the buffer has been sent. */
	xSemaphoreTake(pbcb->semaphore, 6000);

	/* Block if vsnprintf is being uses by someone else. */
	xSemaphoreTake( vsnprintfSemaphoreHandle, portMAX_DELAY );

	/* Construct line of data.  Stop filling buffer if it is full. */
	va_start(argp, fmt);
	va_start(argp, fmt);
	pbcb->size = vsnprintf((char*)(pbcb->pbuf+2),pbcb->maxsize, fmt, argp);
	va_end(argp);

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

