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
	uint32_t size;			// Number of items in struct
 	uint32_t crc;			// crc-32 placed by loader
	uint32_t version;		// struct version number

/* NOTE: 
   - all suffix _t parameters are times in milliseconds
   - all voltages are in volts; prefix 'f' designates float */
	

/* Command/Keep-alive CAN msg timeout duration. */
	uint32_t ka_t;       // (e.g. 1000 ms)

/* uart RX keep-alive timeout duration. */
   uint32_t ksRX_t;     // (e.g. 10 ms)


/* Message timings. */
	uint32_t keepalive_t;// keep-alive timeout (timeout delay ms)
	uint32_t hbct1_t;		// Heartbeat ct: ticks between sending msgs hv1:cur1
	uint32_t hbct2_t;		// Heartbeat ct: ticks between sending msgs hv2:cur2
	uint32_t hbct3_t;		// Heartbeat ct: ticks between sending msgs hv3 (if two gevcus)


/* Send CAN ids  */
	uint32_t cid_hb1;    // CANID-Heartbeat msg volt1:cur1 (volts:amps)
	uint32_t cid_hb2;    // CANID-Heartbeat msg volt2:cur2 (volts:amps)
   uint32_t cid_msg1;   // CANID-gevcu poll response msg: volt1:cur1 (volts:amps)
   uint32_t cid_msg2;   // CANID-gevcu poll response msg: volt2:cur2 (volts:amps)
	uint32_t cid_cmd_r;  // CANID_CMD_CNTCTR1R
	uint32_t cid_keepalive_r; // CANID-keepalive response (status)

/* Receive CAN ids List of CAN ID's for setting up hw filter */
	uint32_t cid_cmd_i;       // CANID_CMD: incoming command
	uint32_t cid_keepalive_i;// CANID-keepalive connect command
	uint32_t cid_gps_sync;    // CANID-GPS time sync msg (poll msg)
	uint32_t code_CAN_filt[5];// Spare
 };

/* *************************************************************************/
void gevcu_idx_v_struct_hardcode_params(struct GEVCULC* p);
/* @brief	: Init struct from hard-coded parameters (rather than database params in highflash)
 * @return	: 0
 * *************************************************************************/
 
#endif

