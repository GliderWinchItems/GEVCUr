/******************************************************************************
* File Name          : shiftregbits.h
* Date First Issued  : 11/13/2019
* Description        : Bit assignments for in & out shift registers
*******************************************************************************/
// SPI_LED assignements (these should become mc system parameters in rewrite)

#ifndef __SHIFTREGBITS
#define __SHIFTREGBITS

/* Sixteen LED or other outputs */
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
#define CL_RST_N0  (1 << 11)	//	low at rest
#define CP_SPARE11 (1 << 10)  //
#define CP_SPARE10 (1 <<  9)  //
#define CL_FS_NO   (1 <<  8)	// low at full scale
#define CP_SPARE8  (1 <<  7)  //
#define CP_SPARE7  (1 <<  6)  //
#define CP_SPARE6  (1 <<  5)  //
#define CP_SPARE5  (1 <<  4)  //
#define CP_SPARE4  (1 <<  3)  //
#define CP_SPARE3  (1 <<  3)  //
#define CP_SPARE2  (1 <<  2)  //
#define CP_SPARE1  (1 <<  0)  //

  
#endif
