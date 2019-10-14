/******************************************************************************
* File Name          : gevcu_idx_v_struct.c
* Date First Issued  : 10/08/2019
* Board              :
* Description        : Load parameter struct
*******************************************************************************/

#include "gevcu_idx_v_struct.h"
#include "SerialTaskReceive.h"

/* *************************************************************************
 * void gevcu_idx_v_struct_hardcode_params(struct struct GEVCULC* p);
 * @brief	: Init struct from hard-coded parameters (rather than database params in highflash)
 * @return	: 0
 * *************************************************************************/
void gevcu_idx_v_struct_hardcode_params(struct GEVCULC* p)
{
	p->size       = 47;
	p->crc        = 0;   // TBD
   p->version    = 1;   // 

	/* Timings in milliseconds. Converted later to timer ticks. */
	p->ka_t       = 2; // Gevcu polling timer (ms) 
	p->keepalive_t= 2555; // keep-alive timeout (timeout delay ms)
	p->hbct1_t    = 1000; // Heartbeat ct: ticks between sending msgs hv1:cur1
	p->hbct2_t    = 1000; // Heartbeat ct: ticks between sending msgs hv2:cur2


   //                 CANID_HEX      CANID_NAME       CAN_MSG_FMT     DESCRIPTION
	p->cid_hb1        = 0xFF800000; // CANID_HB_CNTCTR1V  : FF_FF : Contactor1: Heartbeat: High voltage1:Current sensor1
	p->cid_hb2        = 0xFF000000; // CANID_HB_CNTCTR1A  : FF_FF : Contactor1: Heartbeat: High voltage2:Current sensor2
   p->cid_msg1       = 0x50400000; // CANID_MSG_CNTCTR1V : FF_FF : Contactor1: poll response: High voltage1:Current sensor1
   p->cid_msg2       = 0x50600000; // CANID_MSG_CNTCTR1A : FF_FF : Contactor1: poll response: battery gnd to: DMOC+, DMOC-
	p->cid_cmd_r      = 0xE3600000; // CANID_CMD_CNTCTR1R : U8_VAR: Contactor1: R: Command response
	p->cid_keepalive_r= 0xE3C00000; // CANID_CMD_CNTCTRKAR: U8_U8 : Contactor1: R KeepAlive response

	// List of CAN ID's for setting up hw filter for incoming msgs
	p->cid_cmd_i        = 0xE360000C; // CANID_CMD_CNTCTR1I: U8_VAR: Contactor1: I: Command CANID incoming
	p->cid_keepalive_i  = 0xE3800000; // CANID_CMD_CNTCTRKAI:U8',    Contactor1: I KeepAlive and connect command
	p->cid_gps_sync     = 0x00400000; // CANID_HB_TIMESYNC:  U8 : GPS_1: U8 GPS time sync distribution msg-GPS time sync msg
	p->code_CAN_filt[0] = 0xFFFFFFFC; // CANID_DUMMY: UNDEF: Dummy ID: Lowest priority possible (Not Used)
	p->code_CAN_filt[1] = 0xFFFFFFFC; // CANID_DUMMY: UNDEF: Dummy ID: Lowest priority possible (Not Used)
	p->code_CAN_filt[2] = 0xFFFFFFFC; // CANID_DUMMY: UNDEF: Dummy ID: Lowest priority possible (Not Used)
	p->code_CAN_filt[3] = 0xFFFFFFFC; // CANID_DUMMY: UNDEF: Dummy ID: Lowest priority possible (Not Used)
	p->code_CAN_filt[4] = 0xFFFFFFFC; // CANID_DUMMY: UNDEF: Dummy ID: Lowest priority possible (Not Used)

	return;
}
