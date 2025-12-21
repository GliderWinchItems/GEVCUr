/******************************************************************************
* File Name          : mastercontroller_states.h
* Date First Issued  : 10/11/2020
* Description        : Defines (super-) states for Master Controller
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

/* 12/12/2025
Payload uint16_t, i.e can.cd.us[0]
us[0] unsigned short (little endian)
  [15:13] Active drum number (0- 7)
  [12: 9] Major state in effect (0 - 15)
  [ 8: 5] Minor state (0 - 15)
  [ 4: 0] Reserved

*/

/* Major states                  */
#define MC_SAFE      (0 << 4)  //  contactor  open
#define MC_PREP      (1 << 4)  //  contactor closed
#define MC_ARMED     (2 << 4)  // 
	#define MCS_TAKEUPSLACK  0 //
	#define MCS_GRNDROLL     1 //
#define MC_GRNDRTN   (3 << 4)  //
#define MC_RAMP      (4 << 4)  //
#define MC_CLIMB     (5 << 4)  //
#define MC_RECOVERY  (6 << 4)  // 
#define MC_RETRIEVE  (7 << 4)  //
#define MC_ABORT     (8 << 4)  //
#define MC_STOP      (9 << 4)  // Apply brake



#endif