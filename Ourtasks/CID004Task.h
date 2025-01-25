/******************************************************************************
* File Name          : CID004Task.h
* Date First Issued  : 01/24/2025
* Description        : Send GPS CAN ID 004 64/sec msg
*******************************************************************************/

#ifndef __CID004TASK
#define __CID004TASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "../../../GliderWinchCommons/embed/svn_common/trunk/db/gen_db.h"
#include "MailboxTask.h"
#include "can_iface.h"
#include "CanTask.h"

struct CID004FUNCTION
{
	struct MAILBOXCAN* pmbx_cid_gps_sync; // CANID_HB_TIMESYNC:  U8 : GPS_1: U8 GPS time sync distribution msg-GPS time sync msg	
	struct CANTXQMSG txqcan; // Can msg
};

/* *************************************************************************/
osThreadId xCID004TaskCreate(uint32_t taskpriority);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: CID004TaskHandle
 * *************************************************************************/

extern osThreadId   CID004TaskHandle;

#endif

