/******************************************************************************
* File Name          : CID004Task.c
* Date First Issued  : 01/24/2025
* Description        : Send GPS CAN ID 004 64/sec msg
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "morse.h"
#include "../../../GliderWinchCommons/embed/svn_common/trunk/db/gen_db.h"
#include "can_iface.h"
#include "CanTask.h"
#include "CID004Task.h"


extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block

#define CID004BIT00 (1<<0) // Task notification bit

osThreadId CID004TaskHandle = NULL;

struct CID004FUNCTION cid004function;

/* *************************************************************************
 * void StartCID004Task(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartCID004Task(void* argument)
{
	struct CID004FUNCTION* p = &cid004function;

	/* Mailbox for CAN ID 004. */
	p->pmbx_cid_gps_sync =  MailboxTask_add(pctl0,CANID_HB_TIMESYNC,NULL,CID004BIT00,0,U8);

	/* Init 004 msg. */
	p->txqcan.can.id = CANID_HB_TIMESYNC;
	p->txqcan.can.dlc = 1;        //
	p->txqcan.pctl = pctl0;   // Control block for CAN module
	p->txqcan.maxretryct = 8; //
	p->txqcan.bits = 0;       //

	uint32_t noteval = 0;    // Receives notification word upon an API notify
	
  /* Infinite loop */
  for(;;)
  {
		/* Wait 1/64th sec, and also see of someone else sending CAN ID 004 msgs. */
		xTaskNotifyWait(0,0xffffffff, &noteval, 8);
		if (noteval == CID004BIT00)
		{ // Here, 004 was discovered on bus!
			while (1==1) osDelay(10000); // Loop forever
		}
		/* Here, no other 004 senders detected. */
		p->txqcan.can.cd.uc[0] += 1; // Tick within second count
		// Queue CAN msg
		xQueueSendToBack(CanTxQHandle, &p->txqcan,4);
	}
}
/* *************************************************************************
 * osThreadId xCID004TaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: CID004TaskHandle
 * *************************************************************************/
osThreadId xCID004TaskCreate(uint32_t taskpriority)
{
	BaseType_t ret = xTaskCreate(&StartCID004Task, "CID004Task",\
     64, NULL, taskpriority, &CID004TaskHandle);
	if (ret != pdPASS) return NULL;

	return CID004TaskHandle;
}


