/******************************************************************************
* File Name          : cdc_rxbuffTaskCAN.c
* Date First Issued  : 10/27/2019
* Description        : Incoming ASCII format CAN msgs from PC -> binary CAN 
*******************************************************************************/
#ifndef CDC_RXBUFFTASKCAN
#define CDC_RXBUFFTASKCAN

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "common_can.h"


#define CDCLINETERMINATOR ('\n')	// EOL for incoming stream

/* usb-cdc buffers. */
#define CDCOUTBUFFSIZE 64	// Size for CDC buffer
#define CDCOUTNUMBUF    3  // Number of CDC buffers

struct CDCOUTBUFF
{
	uint32_t len;               // Number of bytes in buffer
	uint8_t u8[CDCOUTBUFFSIZE]; // Buffer
};

struct CDCRXCANMSG
{
	struct CANRCVBUF can;
	uint8_t binseq;         // Received sequence number (binary)
	uint8_t error;          // Error code: 0 = no errors
};

/* *****************************************************************************/
osThreadId xCdcRxTaskReceiveCANCreate(uint32_t taskpriority, uint32_t nb);
/* @brief	: Create task to serve calling task
 * @param	: taskpriority = Task priority (just as it says!)
 * @param	: nb = notification bit used with calling task
 * @return	: Handle to crearted task
 * ****************************************************************************/
struct CDCRXCANMSG* cdc_rxbuffCAN_getCAN(void);
/*	@brief	: Get pointer to CAN msg (w error and sequence)
 * @return	: NULL = no new data, otherwise pointer to struct
  ******************************************************************************/

extern osThreadId CdcRxTaskReceiveCANHandle;

extern struct CDCOUTBUFF cdcbuf[CDCOUTNUMBUF];
extern struct CDCOUTBUFF* pcdcbuf_add;
extern struct CDCOUTBUFF* pcdcbuf_take;

#endif
