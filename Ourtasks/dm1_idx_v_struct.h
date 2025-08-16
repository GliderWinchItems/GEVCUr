/******************************************************************************
* File Name          : dm1_idx_v_struct.h
* Date First Issued  : 01/18/2025
* Description        : DMOC function parameters
*******************************************************************************/

#ifndef __DM1IDXVSTRUCT
#define __DM1IDXVSTRUCT

#include <stdint.h>
#include <math.h>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "CanTask.h"
#include "controlpanel_items.h"
#include "mastercontroller_states.h"

/*
01/17/2025 email: The modes that come to mind are--
1. CL controls torque w speed limit, PB for reversal
2. CL controls speed w torque limit, PB for reversal
3. CL controls speed w torque limit, PB for reversal, 
4. CL controls speed w torque limit, PB for cruise control of speed
5. Auto speed between speed limits with +/- torque limits, CL scales torques
6. CL controls torque w speed limits, w square wave of torque added

*/

#define DMOCMODE_LAW0_MANUAL      0 // Simple torque scaled by CL, w Pushbutton reversal
#define DMOCMODE_LAW1_CLOSEDLOOP  1 // Original pi_loop2 PID speed control
#define DMOCMODE_LAW2_SPEEDLOCK   2 // CL scales speed; pushbutton locks speed
#define DMOCMODE_LAW3_AUTOINERTIA 3 // Back and forth between two speed limits with torque settings

/* Parameters for DMOC control.  C = Local Copy. */
struct DM1_LC
{
   /* Have GEVCUr code skip features not present hardware. */
   uint8_t demoproxy; // 0 = demo winch; 1 = proxy

   uint8_t law_mode; // Control mode
   // Pushbutton index for step?
   // PUshbutton index for select?

   /* The following are initialized according to the mode set. Not all used in each mode. */
   uint32_t maxregenwatts;    // E.g. 60000
   uint32_t maxaccelwatts;    // E.g. 60000
   int32_t  maxspeed_pos;     // Max speed (signed) (e.g. 9000)
   int32_t  maxspeed_neg;     // Max speed (signed) (e.g.-9000)
   float   fmaxtorque_pos;    // Max torque (Nm) forward (e.g. 300)
   float   fmaxtorque_neg;    // Max torque (Nm) reverse (e.g. -300)
   float   fmaxtorque_pos_1;  // Max torque (Nm) forward 1 for auto-inertia (e.g. 300)
   float   fmaxtorque_neg_1;  // Max torque (Nm) reverse 1 for auto-inertia (e.g. -300)
   float   fmaxtorque_pos_2;  // Max torque (Nm) forward 2 for auto-inertia (e.g. 300)
   float   fmaxtorque_neg_2;  // Max torque (Nm) reverse 2 for auto-inertia (e.g. -300)
   int32_t upper_speed_lmt;   // Upper speed limit for auto-inertia
   int32_t lower_speed_lmt;   // Lower speed limit for auto-inertia
   int32_t fwd_kink_speed;    // Upper kink speed for auto-inertia
   int32_t rev_kink_speed;    // Lower kink speed for auto-inertia

   // The following are apply to control_law_v1 (pi_loop)
   float kp;      // Proportional constant
   float ki;      // Integral constant
   float clpi;    // integrator anti-windup clip level
   float clpc;    // command clip level
   float fllspd;  // 100% control lever speed magnitude

   /* The following apply to all modes. */
   uint32_t torqueoffset; // Offset for zero torque,     (nominally 30000)
   uint32_t speedoffset;  // Offset for zero speed       (nominally 20000)
   uint32_t currentoffset;// Offset for reported current (nominally  5000)
};

/* *************************************************************************/
 void dm1_idx_v_struct_hardcode_params(struct DM1_LC* p, uint8_t mode);
/* @brief   : Init struct from hard-coded parameters (rather than database params in highflash)
 * @param   : p = pointer to parameter struct
 * @param   : lmode = parameters for law_mode
 * *************************************************************************/

#endif
