/******************************************************************************
* File Name          : 05200000_dm1_idx_v_struct.c
* Date First Issued  : 01/18/2025
* Description        : DMOC function parameters
* Usage              : 
*******************************************************************************/
#include "dm1_idx_v_struct.h"
#include "SerialTaskReceive.h"
#include "morse.h"
#include "../../../GliderWinchCommons/embed/svn_common/trunk/db/gen_db.h"
#include "control_law_v1.h"

/* *************************************************************************
 * void dm1_idx_v_struct_hardcode_params(struct DM1_LC* p, uint8_t mode);
 * @brief	: Init struct from hard-coded parameters (rather than database params in highflash)
 * @param   : p = pointer to parameter struct
 * @param   : lmode = parameters for law_mode
 * *************************************************************************/
void dm1_idx_v_struct_hardcode_params(struct DM1_LC* p, uint8_t lmode)
{
   switch (lmode)
   {
   case DMOCMODE_LAW0_MANUAL: // 0 Default: Manual. CL controls torque.
      p->maxspeed_pos   =  2500; // Max speed (signed) (e.g. 9000)
      p->maxspeed_neg   = -2500; // Max speed (signed) (e.g.-9000)
      p->fmaxtorque_pos =   30; // Max torque (Nm) forward (e.g. 300)
      p->fmaxtorque_neg =  -30; // Max torque (Nm) reverse (e.g. -300)
      p->maxregenwatts  = 60000; // E.g. 60000
      p->maxaccelwatts  = 60000; // E.g. 60000
      break;

   case DMOCMODE_LAW1_CLOSEDLOOP: // 3  
      p->maxspeed_pos   =  2500; // Max speed (signed) (e.g. 9000)
      p->maxspeed_neg   = -2500; // Max speed (signed) (e.g.-9000)
      p->fmaxtorque_pos =    30; // Max torque (Nm) forward (e.g. 300)
      p->fmaxtorque_neg =   -30; // Max torque (Nm) reverse (e.g. -300)
      p->maxregenwatts  = 60000; // E.g. 60000
      p->maxaccelwatts  = 60000; // E.g. 60000  
      control_law_v1_init();
      break;

   case DMOCMODE_LAW2_SPEEDLOCK:
      p->maxspeed_pos   =  2500; // Max speed (signed) (e.g. 9000)
      p->maxspeed_neg   = -2500; // Max speed (signed) (e.g.-9000)
      p->fmaxtorque_pos =    30; // Max torque (Nm) forward (e.g. 300)
      p->fmaxtorque_neg =   -30; // Max torque (Nm) reverse (e.g. -300)
      p->maxregenwatts  = 60000; // E.g. 60000
      p->maxaccelwatts  = 60000; // E.g. 60000      
      break;

   case DMOCMODE_LAW3_AUTOINERTIA: // 2 Back & forth for inertia measurement
      p->maxspeed_pos   =  1500; // Max speed (signed)
      p->maxspeed_neg   = -1500; // Max speed (signed)
      p->fmaxtorque_pos =   30; // Max torque (Nm) forward
      p->fmaxtorque_neg =   -30; // Max torque (Nm) reverse
      p->maxregenwatts  = 60000; // E.g. 60000
      p->maxaccelwatts  = 60000; // E.g. 60000
      break;


   default: // Mode called for does not have initialization code
      morse_trap (874); // 
      break;
   }

   p->torqueoffset   = 30000; // Offset for zero torque      (nominally 30000)
   p->speedoffset    = 20000; // Offset for zero speed       (nominally 20000)
   p->currentoffset  =  5000; // Offset for reported current (nominally  5000)

   #if 0
// CAN ids encoder function
   //                         CANID_NAME      CANID_HEX  CAN_MSG_FMT     DESCRIPTION
   // We send; Others receive
   p->cid_unit_encoder  = CANID_UNIT_ENCODER2; // 83400000 U8_VAR DiscoveryF4 encoder proxy
   p->cid_msg1_encoder  = CANID_MSG1_ENCODER2; // 83C00000 FF_FF  DiscoveryF4 encoder demo winch: lineout, speed
   p->cid_msg2_encoder  = CANID_MSG2_ENCODER2; // 84000000 FF_FF  DiscoveryF4 encoder demo winch: accel, encoder speed
   p->cid_msg3_encoder  = CANID_MSG3_ENCODER2; // 84400000 FF_S32 DiscoveryF4 encoder demo winch: drum speed, encoder counter

  /* Enable sending of these msgs (which may be at high rate). 1 = enable; 0 = disable. */
   p->msg_enable[0] = 1; // MSG1
   p->msg_enable[1] = 1; // MSG2
   p->msg_enable[2] = 1; // MSG3

   // We receive
   p->cid_gps_sync         = CANID_HB_TIMESYNC;  // 00400000 U8     GPS time sync distribution msg-GPS time sync msg
   p->cid_mc_state         = CANID_MC_STATE;     // 26000000 MC     MC Launch state msg
   p->cid_cmd_encoder      = CANID_CMD_ENCODER2; // 83800000 U8_VAR DiscoveryF4 encoder proxy: command
   p->cid_cmd_uni_bms_pc_i = CANID_UNI_BMS_PC_I; // AEC00000 PC  UNIversal From PC, Used for CAN loading reset');
#endif

	return;
}
