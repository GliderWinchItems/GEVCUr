/******************************************************************************
* File Name          : lcdprintf.c
* Date First Issued  : 10/01/2019
* Description        : LCD display printf
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"

#include "LcdTask.h"
#include "yprintf.h"

#include "stm32f4xx_hal_usart.h"
#include "stm32f4xx_hal_uart.h"

#define LCDQSIZE 8	// LCD Q size

//static uint32_t LcdTaskBuffer[ 64 ];

//static osStaticThreadDef_t LcdTaskControlBlock;

osThreadId LcdTaskHandle = NULL;

static UART_HandleTypeDef* phuart;	// Local copy of uart handle for LCD

/* Queue */
#define QUEUESIZE 4	// Total size of bcb's tasks can queue up
osMessageQId LcdTaskSendQHandle;

/* *************************************************************************
 * void StartLcdTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartLcdTask(void* argument1)
{
	BaseType_t Qret;	// queue receive return

	/* buffer size limited to LCD line length+2+2 */
	struct SERIALSENDTASKBCB* pbuf1 = getserialbuf(puart,24);
	if (pbuf1 == NULL) morse_trap(11);

	struct SERIALSENDTASKBCB*  pssb; // Copied item from queue
	struct SSCIRBUF* ptmp;	// Circular buffer pointer block pointer

  /* Infinite loop */
  for(;;)
  {
		do
		{
		/* Wait indefinitely for someone to load something into the queue */
		/* Skip over empty returns, and NULL pointers that would cause trouble */
			Qret = xQueueReceive(LcdTaskSendQHandle,&pssb,portMAX_DELAY);
			if (Qret == pdPASS) // Break loop if not empty
				break;
		} while ((pssb->phuart == NULL) || (pssb->tskhandle == NULL));

		/* Queue buffer to be sent. */
		vSerialTaskSendQueueBuf(&pssb);
	}
	return;
}
/* *************************************************************************
 * osThreadId xLcdTaskCreate(uint32_t taskpriority, UART_HandleTypeDef* phuart, uint32_t numbcb);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @param	: phuart = pointer to usart control block
 * @param	: numbcb = number of buffer control blocks to allocate
 * @return	: LcdTaskHandle
 * *************************************************************************/
osThreadId xLcdTaskCreate(uint32_t taskpriority, UART_HandleTypeDef* phuart, uint32_t numbcb)
{
	BaseType_t ret = xTaskCreate(StartLcdTask, "LcdTask",\
     192, NULL, taskpriority,\
     &LcdTaskHandle);
	if (ret != pdPASS) return NULL;

	LcdTaskSendQHandle = xQueueCreate(LCDQSIZE, sizeof(struct BEEPQ) );
	if (LcdTaskSendQHandle == NULL) return NULL;

	/* Add bcb circular buffer to SerialTaskSend for usart1 */
	#define NUMCIRBCB1  16 // Size of circular buffer of BCB for usart6
	ret = xSerialTaskSendAdd(&phuart, numbcb, 1); // dma
	if (ret < 0) return NULL;

	return LcdTaskHandle;
}
/* **************************************************************************************
 * int lcdprintf_init(void);
 * @brief	: 
 * @return	: 
 * ************************************************************************************** */
struct SERIALSENDTASKBCB* lcdprintf_init(void)
{
	yprintf_init();	// JIC not init'd
	return;
}
/* **************************************************************************************
 * int yprintf(struct SERIALSENDTASKCB** ppbcb, const char *fmt, ...);
 * @brief	: 'printf' for uarts
 * @param	: pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
 * @param	: format = usual printf format
 * @param	: ... = usual printf arguments
 * @return	: Number of chars "printed"
 * ************************************************************************************** */
int lcdprintf(struct SERIALSENDTASKBCB** ppbcb, int row, int col, const char *fmt, ...)
{
	struct SERIALSENDTASKBCB* pbcb = *ppbcb;
	va_list argp;

//	yprintf_init();	// JIC not init'd

	/* Block if this buffer is not available. SerialSendTask will 'give' the semaphore 
      when the buffer has been sent. */
	xSemaphoreTake(&bbuf1.semaphore, 6000);

	/* Block if this buffer is not available. SerialSendTask will 'give' the semaphore 
      when the buffer has been sent. */
	xSemaphoreTake(pbcb->semaphore, 6000);

	/* Block if vsnprintf is being uses by someone else. */
	xSemaphoreTake( vsnprintfSemaphoreHandle, portMAX_DELAY );

	/* Construct line of data.  Stop filling buffer if it is full. */
	va_start(argp, fmt);
	va_start(argp, fmt);
	pbcb->size = vsnprintf((char*)(pbcb->pbuf),pbcb->maxsize, fmt, argp);
	va_end(argp);

	/* Release semaphore controlling vsnprintf. */
	xSemaphoreGive( vsnprintfSemaphoreHandle );

	/* Limit byte count in BCB to be put on queue, from vsnprintf to max buffer sizes. */
	if (pbcb->size > pbcb->maxsize) 
			pbcb->size = pbcb->maxsize;
#ifdef SKIPPINGFORTESTINH
	/* Set row & column codes */
	char* p = pcbcb->pbuf;
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
#endif
	/* JIC */
	if (pbcb->size == 0) return 0;

	/* Place Buffer Control Block on queue to LcdTask */
		qret=xQueueSendToBack(LcdTaskSendQHandle, ppbcb, portMAX_DELAY);
		if (qret == errQUEUE_FULL) osDelay(1); // Delay, don't spin.

	return pbcb->size;
}

