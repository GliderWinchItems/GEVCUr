/******************************************************************************
* File Name          : gevcu_func_init.c
* Date First Issued  : 10/09/2019
* Description        : uart input
*******************************************************************************/

#include "gevcu_func_init.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "task.h"
#include "MailboxTask.h"
#include "CanTask.h"
#include "can_iface.h"
#include "can_iface.h"
#include "CanTask.h"
#include "GevcuTask.h"
#include "main.h"
#include "stm32f4xx_hal_tim.h"
#include "morse.h"
#include "canfilter_setup.h"

/* From 'main.c' */
extern struct CAN_CTLBLOCK* pctl0;	// Pointer to CAN1 control block
extern TIM_HandleTypeDef htim4;
extern CAN_HandleTypeDef hcan1;

/* *************************************************************************
 * void gevcu_func_init_init(struct GEVCUFUNCTION* p, struct ADCFUNCTION* padc);
 *	@brief	: Initialize working struct for ContactorTask
 * @param	: p    = pointer to ContactorTask
 * @param	: padc = pointer to ADC working struct
 * *************************************************************************/

void gevcu_func_init_init(struct GEVCUFUNCTION* p, struct ADCFUNCTION* padc)
{
	int i;

	/* Pointer to ADC working parameters. */
	p->padc = padc;

	/* Convert ms to timer ticks. */
p->ka_k        = pdMS_TO_TICKS(p->lc.ka_t);        // Command/Keep-alive CAN msg timeout duration.
p->keepalive_k = pdMS_TO_TICKS(p->lc.keepalive_t); // keep-alive timeout (timeout delay ticks)
p->hbct1_k     = pdMS_TO_TICKS(p->lc.hbct1_t);     // Heartbeat ct: ticks between sending msgs hv1:cur1
p->hbct2_k     = pdMS_TO_TICKS(p->lc.hbct2_t);     // Heartbeat ct: ticks between sending msgs hv2:cur2

	/* Add CAN Mailboxes                         CAN           CAN ID              Notify bit   Paytype */
	p->pmbx_cid_cmd_i       =  MailboxTask_add(pctl0,p->lc.cid_cmd_i,      NULL,CNCTBIT06,0,36);
	p->pmbx_cid_keepalive_i =  MailboxTask_add(pctl0,p->lc.cid_keepalive_i,NULL,CNCTBIT07,0,23);
	p->pmbx_cid_gps_sync    =  MailboxTask_add(pctl0,p->lc.cid_gps_sync,   NULL,CNCTBIT08,0,23);


	/* Pre-load fixed data in CAN msgs */
	for (i = 0; i < NUMCANMSGS; i++)
	{
		p->canmsg[i].pctl = pctl0;   // Control block for CAN module (CAN 1)
		p->canmsg[i].maxretryct = 8; //
		p->canmsg[i].bits = 0;       //
		p->canmsg[i].can.dlc = 8;    // Default payload size (might be modified when loaded and sent)
	}

	// Pre-load CAN ids
	p->canmsg[CID_KA_R ].can.id  = p->lc.cid_keepalive_r;
	p->canmsg[CID_MSG1 ].can.id  = p->lc.cid_msg1;
	p->canmsg[CID_MSG2 ].can.id  = p->lc.cid_msg2;
	p->canmsg[CID_CMD_R].can.id  = p->lc.cid_cmd_r;
	p->canmsg[CID_HB1  ].can.id  = p->lc.cid_hb1;
	p->canmsg[CID_HB2  ].can.id  = p->lc.cid_hb2;

	return;
}
/* *************************************************************************
 * void gevcu_func_init_canfilter(struct GEVCUFUNCTION* p);
 *	@brief	: Setup CAN hardware filter with CAN addresses to receive
 * @param	: p    = pointer to ContactorTask
 * *************************************************************************/
void gevcu_func_init_canfilter(struct GEVCUFUNCTION* p)
{
/*	HAL_StatusTypeDef canfilter_setup_add32b_id(uint8_t cannum, CAN_HandleTypeDef *phcan, \
    uint32_t id,   \
    uint8_t  fifo );
 @brief	: Add a 32b id, advance bank number & odd/even
 * @param	: cannum = CAN module number 1, 2, or 3
 * @param	: phcan = Pointer to HAL CAN handle (control block)
 * @param	: id    = 32b CAN id
 * @param	: fifo  = fifo: 0 or 1
 * @return	: HAL_ERROR or HAL_OK
*/
	HAL_StatusTypeDef ret;

	// CANID_CMD_CNTCTR1I: U8_VAR: Contactor1: I: Command CANID incoming
	ret = canfilter_setup_add32b_id(1,&hcan1,p->lc.cid_cmd_i,0);
	if (ret == HAL_ERROR) morse_trap(61);	

	// CANID_CMD_CNTCTRKAI:U8',    Contactor1: I KeepAlive and connect command
	ret = canfilter_setup_add32b_id(1,&hcan1,p->lc.cid_keepalive_i,0);
	if (ret == HAL_ERROR) morse_trap(62);	

	// CANID_HB_TIMESYNC:  U8 : GPS_1: U8 GPS time sync distribution msg-GPS time sync msg
	ret = canfilter_setup_add32b_id(1,&hcan1,p->lc.cid_gps_sync,0);
	if (ret == HAL_ERROR) morse_trap(63);	

	return;
}


