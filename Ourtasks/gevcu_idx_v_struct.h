/******************************************************************************
* File Name          : gevcu_idx_v_struct.h
* Date First Issued  : 10/08/2019
* Board              :
* Description        : Load parameter struct
*******************************************************************************/

#include <stdint.h>
#include "common_can.h"
#include "iir_filter_lx.h"
#include "GevcuTask.h"

#ifndef __GEVCU_IDX_V_STRUCT
#define __GEVCU_IDX_V_STRUCT

/* Parameters gevcu instance */
struct GEVCULC
 {

/* NOTE: all suffix _t parameters are times in milliseconds */

	uint32_t size;
	uint32_t crc;   // TBD
   uint32_t version;   // 

	/* Timings in milliseconds. Converted later to timer ticks. */
	uint32_t ka_t; // Gevcu polling timer (ms) 
	uint32_t keepalive_t; // keep-alive timeout (timeout delay ms)
	uint32_t hbct_t; // Heartbeat ct: ticks between sending 

 // CAN ids we send
   //                                  CANID_NAME             CAN_MSG_FMT     DESCRIPTION
   // Contactor receives; we send
	uint32_t cid_cntctr_keepalive_i; // CANID_CMD_CNTCTRKAI:U8',    Contactor1: I KeepAlive and connect command
  // DMOC receives these commands
	uint32_t cid_dmoc_cmd_speed;    // CANID_DMOC_CMD_SPEED: I16_X6,         DMOC: cmd: speed, key state
	uint32_t cid_dmoc_cmd_torq;     // CANID_DMOC_CMD_TORQ:  I16_I16_I16_X6, DMOC: cmd: torq,copy,standby,status
	uint32_t cid_dmoc_cmd_regen;    // CANID_DMOC_CMD_REGEN: I16_I16_X_U8_U8,DMOC: cmd: watt,accel,degC,alive

 // List of CAN ID's for setting up hw filter for incoming msgs

   // Contactor sends; we receive
	uint32_t cid_cntctr_keepalive_r; // CANID_CMD_CNTCTRKAR: U8_VAR: Contactor1: R KeepAlive response to poll
   // DMOC sends; we receive
	uint32_t cid_dmoc_actualtorq; // CANID_DMOC_ACTUALTORQ:I16,   DMOC: Actual Torque: payload-30000
	uint32_t cid_dmoc_speed;      // CANID_DMOC_SPEED:     I16_X6,DMOC: Actual Speed (rpm?)
	uint32_t cid_dmoc_dqvoltamp;  // CANID_DMOC_DQVOLTAMP: I16_I16_I16_I16','DMOC: D volt:amp, Q volt:amp
	uint32_t cid_dmoc_torque;     // CANID_DMOC_TORQUE:    I16_I16,'DMOC: Torque,-(Torque-30000)
	uint32_t cid_dmoc_critical_f; // CANID_DMOC_TORQUE:    NONE',   'DMOC: Critical Fault: payload = DEADB0FF
	uint32_t cid_dmoc_hv_status;  // CANID_DMOC_HV_STATUS: I16_I16_X6,'DMOC: HV volts:amps, status
	uint32_t cid_dmoc_hv_temps;   // CANID_DMOC_HV_TEMPS:  U8_U8_U8,  'DMOC: Temperature:rotor,invert,stator
   // Others send; we receive
	uint32_t cid_gps_sync; // CANID_HB_TIMESYNC:  U8 : GPS_1: U8 GPS time sync distribution msg-GPS time sync msg

 };

/* *************************************************************************/
void gevcu_idx_v_struct_hardcode_params(struct GEVCULC* p);
/* @brief	: Init struct from hard-coded parameters (rather than database params in highflash)
 * @return	: 0
 * *************************************************************************/
 
#endif

