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
	p->hbct_t    = 1000; // Heartbeat ct: ticks between sending 

 // CAN ids we send
   //                      CANID_HEX      CANID_NAME             CAN_MSG_FMT     DESCRIPTION
	p->cid_cntctr_keepalive_i  = 0xE3800000; // CANID_CMD_CNTCTRKAI:U8',    Contactor1: I KeepAlive and connect command
  // DMOC receives these commands
	p->cid_dmoc_cmd_speed  = 0x46400000;   // CANID_DMOC_CMD_SPEED: I16_X6,         DMOC: cmd: speed, key state
	p->cid_dmoc_cmd_torq   = 0x46600000;   // CANID_DMOC_CMD_TORQ:  I16_I16_I16_X6, DMOC: cmd: torq,copy,standby,status
	p->cid_dmoc_cmd_regen  = 0x46800000;   // CANID_DMOC_CMD_REGEN: I16_I16_X_U8_U8,DMOC: cmd: watt,accel,degC,alive

 // List of CAN ID's for setting up hw filter for incoming msgs
   // Contactor sends
	p->cid_cntctr_keepalive_r = 0xE3600000; // CANID_CMD_CNTCTRKAR: U8_VAR: Contactor1: R KeepAlive response to poll
   // DMOC sends
	p->cid_dmoc_actualtorq = 0x47400000; // CANID_DMOC_ACTUALTORQ:I16,   DMOC: Actual Torque: payload-30000
	p->cid_dmoc_speed      = 0x47600000; // CANID_DMOC_SPEED:     I16_X6,DMOC: Actual Speed (rpm?)
	p->cid_dmoc_dqvoltamp  = 0x47C00000; // CANID_DMOC_DQVOLTAMP: I16_I16_I16_I16','DMOC: D volt:amp, Q volt:amp
	p->cid_dmoc_torque     = 0x05683004; // CANID_DMOC_TORQUE:    I16_I16,'DMOC: Torque,-(Torque-30000)
	p->cid_dmoc_critical_f = 0x056837fc; // CANID_DMOC_TORQUE:    NONE',   'DMOC: Critical Fault: payload = DEADB0FF
	p->cid_dmoc_hv_status  = 0xCA000000; // CANID_DMOC_HV_STATUS: I16_I16_X6,'DMOC: HV volts:amps, status
	p->cid_dmoc_hv_temps   = 0xCA200000; // CANID_DMOC_HV_TEMPS:  U8_U8_U8,  'DMOC: Temperature:rotor,invert,stator
   // Others send
	p->cid_gps_sync     = 0x00400000; // CANID_HB_TIMESYNC:  U8 : GPS_1: U8 GPS time sync distribution msg-GPS time sync msg



	return;
}
