/******************************************************************************
* File Name          : GatewayTask.c
* Date First Issued  : 02/25/2019
* Description        : PC<->gateway using usart2, notified by MailboxTask
*******************************************************************************/
/*
  02/26/2019
This task works in conjunction with 'MailboxTask'.  'MailboxTask' notifies this
task when a CAN module circular buffer has one or more CAN msgs.  This task directly
accesses the circular buffer via 'take' pointer.

The 'MailboxTask' is likely a high FreeRTOS priority task.  This task might run at
a lower priority since timing is not critical, however delays require that the 
circular buffer be large enough to avoid overrun since there is no protection for
buffer overflow (since CAN msgs are added to the circular buffer under interrupt).

This version only handles PC->CAN bus msgs for CAN1 module.  To mix CAN1 and CAN2
requires implementing the scheme of commandeering the low order bit(s) from the
sequence number byte.

CAN->PC direction CAN1 and CAN2 msgs are mixed together, except for the cases where
the CANIDs are identical such as DMOC msgs, in which case the CAN2 msgs are tagged 
as 29b address. [04/06/2019--not implemented]
*/
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_can.h"
#include "GatewayTask.h"
#include "can_iface.h"
#include "MailboxTask.h"
#include "getserialbuf.h"
#include "SerialTaskSend.h"
#include "morse.h"
#include "yprintf.h"
#include "SerialTaskReceive.h"
#include "gateway_PCtoCAN.h"
#include "gateway_CANtoPC.h"
#include "main.h"
#include "cdc_txbuff.h"
//#include "cdc_rxbuff.h"
#include "cdc_rxbuffTaskCAN.h"

uint32_t dbuggateway1;

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;

/* from 'main' */
extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block
extern struct CAN_CTLBLOCK* pctl1;	// Pointer to CAN2 control block

void StartGatewayTask(void const * argument);

osThreadId GatewayTaskHandle;

/* A notification to Gateway copies the internal notification word to this. */
uint32_t GatewayTask_noteval = 0;    // Receives notification word upon an API notify

/* *************************************************************************
 * osThreadId xGatewayTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: GatewayHandle
 * *************************************************************************/
osThreadId xGatewayTaskCreate(uint32_t taskpriority)
{
 /* definition and creation of CanTask */
   osThreadDef(GatewayTask, StartGatewayTask, osPriorityNormal, 0, (272));
   GatewayTaskHandle = osThreadCreate(osThread(GatewayTask), NULL);
	vTaskPrioritySet( GatewayTaskHandle, taskpriority );

	return GatewayTaskHandle;
}
/* *************************************************************************
 * void StartGatewayTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
uint8_t gatercvflag = 0;

void StartGatewayTask(void const * argument)
{
//osDelay(100); // Debugging

	int i;

	/* The lower order bits are reserved for incoming CAN module msg notifications. */
	#define TSKGATEWAYBITc1	 (1 << (STM32MAXCANNUM + 2))  // Task notification bit for huart2 incoming ascii CAN
	#define TSKGATEWAYBITCDC (1 << (STM32MAXCANNUM + 3))  // Task notification bit for CDC_OUT (read via usb)

	/* notification bits processed after a 'Wait. */
	uint32_t noteused = 0;

	struct SERIALRCVBCB* prbcb2;	// usart2 (PC->CAN msgs)
	struct CANRCVBUFPLUS* pcanp;  // Basic CAN msg Plus error and seq number
	struct CANRCVBUFN* pncan;

	/* PC, or other CAN, to CAN msg */
	// Pre-load fixed elements for queue to CAN 'put' 

#ifdef CONFIGCAN2 // CAN2 implemented
	// CAN1
	struct CANTXQMSG canqtx1;
	canqtx1.pctl       = pctl0;
	canqtx1.maxretryct = 8;
	canqtx1.bits       = 0; // /NART
#endif

   // CAN2
	struct CANTXQMSG canqtx2;
	canqtx2.pctl = pctl1;
	canqtx2.maxretryct = 8;
	canqtx2.bits       = 0; // /NART

	// PC -> CAN1 (no PC->CAN2)
	struct CANTXQMSG pccan1;
	pccan1.pctl       = pctl0;
	pccan1.maxretryct = 8;
	pccan1.bits       = 0; // /NART

	/* Setup serial output buffers for uarts. */
	struct SERIALSENDTASKBCB* pbuf2 = getserialbuf(&HUARTMON,  96); // PC monitor uart
	struct SERIALSENDTASKBCB* pbuf3 = getserialbuf(&HUARTGATE,128); // Gateway uart, CAN1

#ifdef CONFIGCAN2 // CAN2 implemented
	struct SERIALSENDTASKBCB* pbuf4 = getserialbuf(&HUARTGATE,128); // Gateway uart, CAN2
#endif

	/* Use usb-cdc for gateway data. */
#ifdef  USEUSBFORCANMSGS
	// CAN1
	struct SERIALSENDTASKBCB* pbuf5 = getserialbuf(&HUARTGATE,128); // Gateway cdc, CAN1
	if (pbuf5 == NULL) morse_trap(70);
	struct CDCTXTASKBCB cdc2;
	cdc2.pbuf = pbuf5->pbuf;

	// PC-cdc -> CAN1 (no PC->CAN2)
	struct CANTXQMSG pccanc;
	pccanc.pctl       = pctl0;
	pccanc.maxretryct = 8;
	pccanc.bits       = 0; // /NART

	struct CDCRXCANMSG* prxcanmsg;

	// CAN2
	#ifdef CONFIGCAN2 // CAN2 implemented
		struct SERIALSENDTASKBCB* pbuf6 = getserialbuf(&HUARTGATE,128); // Gateway cdc, CAN2
		if (pbuf6 == NULL) morse_trap(71);
		struct CDCTXTASKBCB cdc3;
		cdc3.pbuf = pbuf6->pbuf;
	#endif

	// CDC receiving (priority, our notification bit)
	osThreadId ret = xCdcRxTaskReceiveCANCreate(1, TSKGATEWAYBITCDC);
	if (ret == NULL) morse_trap(84);

#endif

	/* Pointers into the CAN  msg circular buffer for each CAN module. */
	struct CANTAKEPTR* ptake[STM32MAXCANNUM] = {NULL};

	/* Setup serial input buffering and line-ready notification */
     //   (ptr uart handle, dma flag, notiification bit, 
     //   ptr notification word, number line buffers, size of lines, 
     //   dma buffer size);
	/* PC-to-CAN ascii/hex incoming "lines" directly converts to CAN msgs. */

	/* WARNING: For the CAN mode, the number of bytes for the line buffer
      must be result in an alignment for double word copying, or else a Hard Fault
      will be thrown. See struct 'CANRCVBUFPLUS' in 'SerialTaskReceive.h'.
      struct CANRCVBUF is four words, but the 'PLUS adds two bytes, so rounding
      up makes 20 the minimum size. */
	prbcb2 = xSerialTaskRxAdduart(&HUARTGATE,1,TSKGATEWAYBITc1,\
		&GatewayTask_noteval,16,20,128,1); // buff 12 CAN, of 20 bytes, 192 total dma, CAN mode
	if (prbcb2 == NULL) morse_trap(41);

gatercvflag = 1;

	/* Get pointers to circular buffer pointers for each CAN module in list. */	
	for (i = 0; i < STM32MAXCANNUM; i++)
	{
		if ((mbxcannum[i].pmbxarray != NULL) && (mbxcannum[i].pctl != NULL))
		{
			ptake[i] = can_iface_add_take(mbxcannum[i].pctl);
			yprintf(&pbuf2,"\n\rStartGateway: mbxcannum[%i] setup OK. array: 0x%08X pctl: 0x%08X",i,mbxcannum[i].pmbxarray,mbxcannum[i].pctl);
		}
		else
		{
			yprintf(&pbuf2,"\n\rStartGateway: mbxcannum[%i] was not setup.",i);
		}
	}

	/* Start CANs */
extern CAN_HandleTypeDef hcan1;
	HAL_CAN_Start(&hcan1); // CAN1
#ifdef CONFIGCAN2
	HAL_CAN_Start(&hcan2); // CAN2
#endif

//taskENTER_CRITICAL();
//  MX_USB_DEVICE_Init();
//taskEXIT_CRITICAL();

  /* Infinite RTOS Task loop */
  for(;;)
  {
		/* Wait for either PC line completion, or 'MailboxTask' notifications. */
		xTaskNotifyWait(noteused, 0, &GatewayTask_noteval, portMAX_DELAY);
		noteused = 0;	// Accumulate bits in 'noteval' processed.

		/* CAN1 incoming msg: Check notification bit */
		i = 0;	// CAN1 index
		{
			if ((GatewayTask_noteval & (1 << i)) != 0)
			{
				noteused |= (GatewayTask_noteval & (1 << i)); // We handled the bit			
				do
				{
					/* Get pointer into CAN msg circular buffer */
					pncan = can_iface_get_CANmsg(ptake[i]);
					if (pncan != NULL)
					{			
					/* Convert binary to the ascii/hex format for PC. */
						canqtx2.can = pncan->can; // Save a local copy
						xSemaphoreTake(pbuf3->semaphore, 5000);
						gateway_CANtoPC(&pbuf3, &canqtx2.can);

					/* === CAN1 -> PC === */			
						vSerialTaskSendQueueBuf(&pbuf3); // Place on queue for usart2 sending

#ifdef USEUSBFORCANMSGS
						// Buffers are independent, so copy it
						memcpy(pbuf5->pbuf,pbuf3->pbuf,pbuf3->size);
						pbuf5->size = pbuf3->size;
						cdc2.size = pbuf5->size;
						xQueueSendToBack(CdcTxTaskSendQHandle,&cdc2,1500);
#endif

#ifdef CONFIGCAN2 // CAN2 setup
					/* === CAN1 -> CAN2 === */
						xQueueSendToBack(CanTxQHandle,&canqtx2,portMAX_DELAY);
#endif
					}
				} while (pncan != NULL);	// Drain the buffer
			}
		}
#ifdef CONFIGCAN2 // CAN2 implemented
		/* CAN2 incoming msg: Check notification bit */
		i = 1;	// CAN2 index
		{
			if ((GatewayTask_noteval & (1 << i)) != 0)
			{
				noteused |= (GatewayTask_noteval & (1 << i)); // We handled the bit			
				do
				{
					/* Get pointer into CAN msg circular buffer */
//					pncan = can_iface_get_CANmsg(ptake[i]);
					pncan = Mailboxgetbuf(i);
					if (pncan != NULL)
					{			
					/* Convert binary to the ascii/hex format for PC. */
						canqtx1.can = pncan->can;	// Save a local copy
						xSemaphoreTake(pbuf4->semaphore, 5000);
						gateway_CANtoPC(&pbuf4, &canqtx1.can);

					/* === CAN2 -> PC === */			
						vSerialTaskSendQueueBuf(&pbuf4); // Place on queue for usart2 sendingpctocanc

   #ifdef USEUSBFORCANMSGS
						// Buffers are independent, so copy it
						memcpy(pbuf6->pbuf,pbuf4->pbuf,pbuf4->size);
						pbuf6->size = pbuf4->size;
						cdc3.size = pbuf6->size;
						xQueueSendToBack(CdcTxTaskSendQHandle,&cdc3,1500);
   #endif


					/* === CAN1 -> CAN2 === */
						xQueueSendToBack(CanTxQHandle,&canqtx1,portMAX_DELAY);
					}
				} while (pncan != NULL);	// Drain the buffer
			}
		}
#endif
		/* PC incoming msg: Handle incoming usart2 carrying ascii/hex CAN msgs */
		if ((GatewayTask_noteval & TSKGATEWAYBITc1) != 0)
		{ // Here, one or more PC->CAN msgs have been received
			noteused |= TSKGATEWAYBITc1; // We handled the bit
			/* Get incoming CAN msgs from PC and queue for output to CAN1 bus. */
			do
			{
				pcanp = gateway_PCtoCAN_getCAN(prbcb2);
				if (pcanp != NULL)
				{
					/* Check for errors */
					if (pcanp->error == 0)
					{
						/* Place CAN msg on queue for sending to CAN bus */
						pccan1.can = pcanp->can;
//	pccan1.can.id = pcanp->can.id;
//	pccan1.can.dlc = pcanp->can.dlc;
//	pccan1.can.cd.ull = pcanp->can.cd.ull;
//	pccan1.can.cd.ui[0] = pcanp->can.cd.ui[0];
//	pccan1.can.cd.ui[1] = pcanp->can.cd.ui[1];

						xQueueSendToBack(CanTxQHandle,&pccan1,portMAX_DELAY);
					}
					else
					{ // Here, one or more errors. List for the hapless Op to ponder
						yprintf(&pbuf2,"\n\r@@@@@ PC CAN ERROR: %i 0X%04X, 0X%08X 0X%02X 0X%08X %i 0X%02X 0X%02X %s",pcanp->seq, pcanp->error,\
							pcanp->can.id,pcanp->can.dlc,pcanp->can.cd.ui[0]);

						/* For test purposes: Place CAN msg on queue for sending to CAN bus */
						pccan1.can = pcanp->can;
						xQueueSendToBack(CanTxQHandle,&pccan1,portMAX_DELAY);
					}
				}
			} while ( pcanp != NULL);
		}
#ifdef USEUSBFORCANMSGS
		if ((GatewayTask_noteval & TSKGATEWAYBITCDC) != 0)
		{ // Here, one or more PC->CAN msgs have been received
			noteused |= TSKGATEWAYBITCDC; // We handled the bit

dbuggateway1 += 1;

			/* Convert and queue CAN msgs until CDC lines comsumed. */
			while ( (prxcanmsg = cdc_rxbuffCAN_getCAN()) != NULL )
			{

				/* Check for errors */
				if (prxcanmsg->error == 0)
				{ // Here, no errors.
					/* Copy to preloaded local buffer. */
					pccanc.can = prxcanmsg->can; // Copy CAN msg to preloaded local struct

					/* Place CAN msg on queue for sending to CAN bus */
					xQueueSendToBack(CanTxQHandle,&pccanc,portMAX_DELAY);
				}
				else
				{ // Here, one or more errors. List for the hapless Op to ponder
					yprintf(&pbuf2,"\n\r@@@@@ PC CDC CAN ERROR: %i 0X%04X, 0X%08X 0X%02X 0X%08X %i 0X%02X 0X%02X %s",prxcanmsg->binseq, prxcanmsg->error,\
						prxcanmsg->can.id,prxcanmsg->can.dlc,prxcanmsg->can.cd.ui[0]);

					/* For test purposes: Place CAN msg on queue for sending to CAN bus */
					pccanc.can = prxcanmsg->can;

					xQueueSendToBack(CanTxQHandle,&pccanc,portMAX_DELAY);
				}
			}
		}
#endif
  }
}

