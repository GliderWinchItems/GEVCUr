/******************************************************************************
* File Name          : mastercontroller_states.h
* Date First Issued  : 10/11/2020
* Description        : Defines states for Master Controller
*******************************************************************************/

#ifndef __MASTERCONTROLLERSTATES
#define __MASTERCONTROLLERSTATES

#include <stdint.h>
/*
10/11/2020 definition
// Master Controller state machine  definitions 
// Lower nibble reserved for sub-states if needed
#define MC_SAFE      (0 << 4)
#define MC_PREP      (1 << 4)
#define MC_ARMED     (2 << 4)
#define MC_GRNDRTN   (3 << 4)
#define MC_RAMP      (4 << 4)
#define MC_CLIMB     (5 << 4)
#define MC_RECOVERY  (6 << 4)
#define MC_RETRIEVE  (7 << 4)
#define MC_ABORT     (8 << 4)
#define MC_STOP      (9 << 4)
*/

/* 12/23/2025
Payload: U8_U8_U8
uc[0] - Major states
  Upper 4 bits: Original states 
  Lower 4 bits: Additional states

uc[1] - sub-states
  Upper 4 bits: Original sub-states
  Lower 4 bits: Additional states

uc[2] - drum
  Upper 4 bits: reserved
  Lower 3 bits: drum number 
*/

struct MC_STATE
{
  uint8_t major; // Major state (MC_...)
  uint8_t sub;   // Sub-state (MCS_...)
  uint8_t drum;  // Drum selected) (0 - 7)
};

/* Major states    12/23/2025              */
#define MC_INIT      ( 0 << 4)  // MC Initialization
  #define MCS_WAIT_READY          ( 0 << 4) // Wait: LCD init, CL to calibrate
  #define MCS_WAIT_SA_SW_SAFE     ( 1 << 4) // Wait: SAFE/ACTIVE to be SAFE

#define MC_SAFE      ( 1 << 4)  //  contactor  open
  #define MCS_SAFE_INIT                 ( 0 << 4) // Open contactor
  #define MCS_SAFE_WAIT_CONTACTOR_OPEN  ( 1 << 4) //

  #define MCS_SAFE_WAIT_SA_SW_ACTIVE    ( 3 << 4) //
  #define MCS_SAFE_WAIT_CONTACTOR_CLOSE ( 4 << 4) //

#define MC_PREP      ( 2 << 4)  //  contactor closed
  #define MCS_PREP_INIT         ( 0 << 4) //
  #define MCS_PREP_TAKEUPSLACK  ( 1 << 4) //
  #define MCS_PREP_GRNDROLL     ( 2 << 4) //

#define MC_ARMED     ( 3 << 4)  // 
#define MC_GRNDRTN   ( 4 << 4)  //
#define MC_RAMP      ( 5 << 4)  //
#define MC_CLIMB     ( 6 << 4)  //
#define MC_RECOVERY  ( 7 << 4)  // 
#define MC_RETRIEVE  ( 8 << 4)  //
#define MC_ABORT     ( 9 << 4)  //
#define MC_STOP      (10 << 4)  // Apply brake

#endif


