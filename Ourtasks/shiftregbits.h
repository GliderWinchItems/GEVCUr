/******************************************************************************
* File Name          : shiftregbits.h
* Date First Issued  : 11/13/2019
* Description        : Bit assignments for in & out shift registers
*******************************************************************************/
// SPI_LED assignements (these should become mc system parameters in rewrite)

#ifndef __SHIFTREGBITS
#define __SHIFTREGBITS

/* Sixteen LED or other outputs */
/* etmc0 definitions */
#define LED_SAFE        0x8000
#define LED_PREP        0x4000
#define LED_ARM         0x2000
#define LED_GNDRLRTN    0x1000
#define LED_RAMP        0x0800
#define LED_CLIMB       0x0400
#define LED_RECOVERY    0x0200
#define LED_RETRIEVE    0x0100
#define LED_STOP        0x0080
#define LED_ABORT       0x0040
#define LED_CL_RST      0x0020
#define LED_SPARE10     0x0010
#define LED_SPARE08     0x0008
#define LED_CL_FS       0x0004
#define LED_PREP_PB     0x0002
#define LED_ARM_PB      0x0001

//	Sixteen control panel switch mapping
#define SW_SAFE    (1 << 15)	//	active low
#define SW_ACTIVE  (1 << 14)	//	active low
#define PB_ARM     (1 << 13)	//	active low
#define PB_PREP    (1 << 12)	//	active low
#define CL_RST_N0  (1 << 11)	//	Control Lever:Rest switch (low) 
#define CP_SPARE3  (1 << 10)  //
#define CP_SPARE10 (1 <<  9)  //
#define CL_FS_NO   (1 <<  8)	// Control Lever:Forward switch (low)
#define CP_BRAKE   (1 <<  7)  // Brake
#define CP_GUILLO  (1 <<  6)  // Guillotine
#define CP_JOGLEFT (1 <<  5)  // Joggle Left
#define CP_JOGRITE (1 <<  4)  // Joggle Right
#define CP_ZODOMTR (1 <<  3)  // Zero Odometer
#define CP_ZTENSION (1 << 2) // Zero tension
#define CP_SPARE1  (1 <<  1)  //
#define CP_SPARE0  (1 <<  0)  //

// GEVCUr switch re-use
#define CP_REVERSETORQ  CP_ZTENSION // Changes sign of torque command


#endif
