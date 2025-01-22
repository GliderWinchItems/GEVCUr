/******************************************************************************
* File Name          : control_law_params.c
* Date First Issued  : 01/22/2025
* Description        : Parameters for control_law routines and tests
* Usage              : 
*******************************************************************************/
#include "SerialTaskReceive.h"
#include "morse.h"
#include "../../../GliderWinchCommons/embed/svn_common/trunk/db/gen_db.h"
#include "dmoc_control.h"
#include "control_law_v1.h"

/* *************************************************************************
 * void dm1_idx_v_struct_hardcode_params(struct DM1_LC* p, uint8_t mode);
 * @brief	: Init struct from hard-coded parameters (rather than database params in highflash)
 * @param   : p = pointer to parameter struct
 * @param   : mode = select parameters for this mode
 * @return	: 0
 * *************************************************************************/
void dm1_idx_v_struct_hardcode_params(struct DM1_LC* p, uint8_t mode)
{
   switch (mode)
   {
   case DMOCMODE_LAW0_MANUAL: // 0 Default: Manual. CL controls torque.
      p->maxspeed_pos   =  5000; // Max speed (signed) (e.g. 9000)
      p->maxspeed_neg   = -5000; // Max speed (signed) (e.g.-9000)
      p->fmaxtorque_pos =   300; // Max torque (Nm) forward (e.g. 300)
      p->fmaxtorque_neg =  -300; // Max torque (Nm) reverse (e.g. -300)
      p->maxregenwatts  = 60000; // E.g. 60000
      p->maxaccelwatts  = 60000; // E.g. 60000
      break;

   case DMOCMODE_LAW1_CLOSEDLOOP: // PI_LOOP2
      control_law_v1_init();
      break;

   case DMOCMODE_LAW2_SPEEDLOCK:
      p->maxspeed_pos   =  4000; // Max speed (signed) (e.g. 9000)
      p->maxspeed_neg   = -4000; // Max speed (signed) (e.g.-9000)
      p->fmaxtorque_pos =    10; // Max torque (Nm) forward (e.g. 300)
      p->fmaxtorque_neg =   -10; // Max torque (Nm) reverse (e.g. -300)
      p->maxregenwatts  = 60000; // E.g. 60000
      p->maxaccelwatts  = 60000; // E.g. 60000      
      break;

   case DMOCMODE_LAW3_AUTOINERTIA: // 2 Back & forth for inertia measurement
      p->maxspeed_pos   =  3000; // Max speed (signed)
      p->maxspeed_neg   = -3000; // Max speed (signed)
      p->fmaxtorque_pos =   100; // Max torque (Nm) forward
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
