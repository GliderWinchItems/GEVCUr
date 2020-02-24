/******************************************************************************
* File Name          : shiftregbits.h
* Date First Issued  : 11/13/2019
* Description        : Bit assignments for spi in & out shift registers
*******************************************************************************/
// SPI_LED assignements (these should become mc system parameters in rewrite)

#ifndef __SHIFTREGBITS
#define __SHIFTREGBITS

/* Sixteen LED or other outputs */
/* etmc0 original definitions 
#define LED_SAFE        15 // 0x8000
#define LED_PREP        14 // 0x4000
#define LED_ARM         13 // 0x2000
#define LED_GNDRLRTN    12 // 0x1000
#define LED_RAMP        11 // 0x0800
#define LED_CLIMB       10 // 0x0400
#define LED_RECOVERY     9 // 0x0200
#define LED_RETRIEVE     8 // 0x0100
#define LED_STOP         7 // 0x0080
#define LED_ABORT        6 // 0x0040
#define LED_CL_RST       5 // 0x0020
#define LED_SPARE10      4 // 0x0010
#define LED_SPARE08      3 // 0x0008
#define LED_CL_FS        2 // 0x0004
#define LED_PREP_PB      1 // 0x0002
#define LED_ARM_PB       0 // 0x0001
*/

/* Revised 2/15/2020 
#define LED_STOP        14 //
#define LED_ABORT       15 //
#define LED_RETRIEVE     0 //
#define LED_RECOVERY     1 //
#define LED_CLIMB        2 //
#define LED_RAMP         3 //
#define LED_GNDRLRTN     4 //
#define LED_ARM          5 //
#define LED_PREP         6 //
#define LED_SAFE         7 //
#define LED_ARM_PB       8 //
#define LED_PREP_PB      9 //
#define LED_SPARERS     10 // *LED_CL_RST
#define LED_SPARE10     11 //
#define LED_SPARE08     12 //
#define LED_SPAREFS     13 // *LED_CL_FS
*/

/* 2/23/2020 gsm CP panel testing */
#define LED_STOP        14 //
#define LED_ABORT       15 //
#define LED_RETRIEVE     0 //
#define LED_RECOVERY     1 //
#define LED_CLIMB        2 //
#define LED_RAMP         3 //
#define LED_GNDRLRTN     4 //
#define LED_ARM          5 //
#define LED_PREP         6 //
#define LED_SAFE         7 //
#define LED_ARM_PB       8 //
#define LED_PREP_PB      9 //
#define LED_SPARERS     10 // *LED_CL_RST
#define LED_SPARE11     11 //
#define LED_SPARE12     12 //
#define LED_SPARE13     13 // *LED_CL_FS


// * - as originally defined, but not implemented

/* Result of test with Control Panel */
// GSM CP test: 1/19/20 FFxx (high byte not connected)
// NOTE: shift reg 'H' bit is sent first, and is spi msb.
// STM32 16b word reverses order of bytes sent, i.e. the
//  output of 0x1122 has the byte with 0x22 sent first.
//                   (low byte) sw off on (on = active low)
#define SW_SAFE     (1 <<  7)	//	F7 77  P8-3  IN 0 H
#define SW_ACTIVE   (1 <<  6)	//	F7 B7  P8-4  IN 1 G
#define PB_ARM      (1 <<  5)	//	77 57  P8-5  IN 2 F
#define PB_PREP     (1 <<  4)	//	77 67  P8-6  IN 3 E
#define CL_RST_N0   (1 <<  3)	//	7F 77  P8-7  IN 4 D
#define CP_ZTENSION (1 <<  2) // 77 73  P8-8  IN 5 C
#define CP_ZODOMTR  (1 <<  1) // 77 75  P8-9  IN 6 B
#define CL_FS_NO    (1 <<  0)	// 7F 7E  P8_10 IN 7 A

// High byte not yet connected (?)
#define CP_SPARE1  (1 << 15)  // Not on connector   IN 15 A
#define CP_SPARE2  (1 << 14)  // Not on connector   IN 14 B
#define CP_DUNNO1  (1 << 13)  // Zero tension P8-16 IN 13 C
#define CP_DUNNO2  (1 << 12)  // Zero odometerP8-15 IN 12 D
#define CP_JOGRITE (1 << 11)  // Joggle Right P8-14 IN 11 E *
#define CP_JOGLEFT (1 << 10)  // Joggle Left  P8-13 IN 10 F *
#define CP_GUILLO  (1 <<  9)  // Guillotine   P8-12 IN  9 G
#define CP_BRAKE   (1 <<  8)  // Brake        P8-11 IN  8 H 
// * - Not connected


// GEVCUr switch re-use
#define CP_REVERSETORQ  CP_ZTENSION // Changes sign of torque command


#endif
