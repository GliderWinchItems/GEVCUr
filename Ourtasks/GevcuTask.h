/******************************************************************************
* File Name          : GevcuTask.h
* Date First Issued  : 06/25/2019
* Description        : Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#ifndef __GEVCUTASK
#define __GEVCUTASK

#include <stdint.h>
#include "FreeRTOS.h"
#include "adcparams.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "adc_idx_v_struct.h"
#include "CanTask.h"

/* 
=========================================      
CAN msgs: 
ID suffix: "_i" = incoming msg; "_r" response msg

RECEIVED msgs directed into contactor function:
 (1) contactor command (also keep-alive) "cid_keepalive_i"
     payload[0]
       bit 7 - connect request
       bit 6 - reset critical error
 (2) poll (time sync) "cid_gps_sync"
 (3) function command (diagnostic poll) "cid_cmd_i"
    
SENT by contactor function:
 (1) contactor command "cid_keepalive_r" (response to "cid_keepalive_i")
     payload[0]
       bit 7 - faulted (code in payload[2])
       bit 6 - warning: minimum pre-chg immediate connect.
              (warning bit only resets with power cycle)
		 bit[0]-[3]: Current main state code

     payload[1] = critical error state error code
         0 = No fault
         1 = battery string voltage (hv1) too low
         2 = contactor 1 de-energized, aux1 closed
         3 = contactor 2 de-energized, aux2 closed
         4 = contactor #1 energized, after closure delay aux1 open
         5 = contactor #2 energized, after closure delay aux2 open
         6 = contactor #1 does not appear closed
         7 = Timeout before pre-charge voltage reached cutoff
         8 = Gevcu #1 closed but voltage across it too big
         9 = Gevcu #2 closed but voltage across it too big

		payload[2]
         bit[0]-[3] - current substate CONNECTING code
         bit[4]-[7] - current substate (spare) code

 poll  (response to "cid_gps_sync") & heartbeat
 (2)  "cid_msg1" hv #1 : current #1  battery string voltage:current
 (3)	"cid_msg2" hv #2 : hv #3       DMOC+:DMOC- voltages

 function command "cid_cmd_r"(response to "cid_cmd_i")
 (4)  conditional on payload[0], for example(!)--
      - ADC ct for calibration purposes hv1
      - ADC ct for calibration purposes hv2
      - ADC ct for calibration purposes hv3
      - ADC ct for calibration purposes current1
      - ADC ct for calibration purposes current2
      - Duration: (Energize coil 1 - aux 1)
      - Duration: (Energize coil 2 - aux 2)
      - Duration: (Drop coil 1 - aux 1)
      - Duration: (Drop coil 2 - aux 2)
      - volts: 12v CAN supply
      - volts: 5v regulated supply
      ... (many and sundry)

 heartbeat (sent in absence of keep-alive msgs)
 (5)  "cid_hb1" Same as (2) above
 (6)  "cid_hb2" Same as (3) above

=========================================    
NOTES:
1. GEVCU DESIGNATION--
   (Important for following GevcuStates.c code)

   a. Two contactor configuration
      #1 Battery string plus-to-DMOC plus
      #2 Battery string minus-to-DMOC minus
          with pre-charge resistor across #2's contacts.

   b. One contactor (with small pre-charge relay)
      #1 Battery string plus-to-DMOC plus
      #2 Battery string plus-to-pre-charge resistor,
          with pre-charge resistor to DMOC plus

2. The command/keep-alive msgs are sent
   - As a response to every command/keep-alive
   - Immediately when the program state changes
   - Every keep-alive timer timeout when incoming keep-alive
     msgs are not being receive, i.e. becomes a status heartbeat.

3. hv3 cannot measure negative voltages which might occur during
   regeneration with contactor #2 closed.  Likewise, hv2 can 
   exceed hv1 so that the difference becomes negative.  In both
   case the negative values would be small.

*/


/* Task notification bit assignments for GevcuTask. */
#define GEVCUBIT00	(1 << 0)  // ADCTask has new readings
#define GEVCUBIT01	(1 << 1)  // spare
#define GEVCUBIT02	(1 << 2)  // spare
#define GEVCUBIT03	(1 << 3)  // Software timer 3 timeout callback
#define GEVCUBIT04	(1 << 4)  // Software timer 1 timeout callback
#define GEVCUBIT05	(1 << 5)  // Software timer 2 timeout callback
// MailboxTask notification bits for CAN msg mailboxes
#define GEVCUBIT06 ( 1 <<  6) // cid_gps_sync
#define GEVCUBIT07 ( 1 <<  7) // cid_cntctr_keepalive_r
#define GEVCUBIT08 ( 1 <<  8) // cid_dmoc_actualtorq
#define GEVCUBIT09 ( 1 <<  9) // cid_dmoc_speed
#define GEVCUBIT10 ( 1 << 10) // cid_dmoc_dqvoltamp
#define GEVCUBIT11 ( 1 << 11) // cid_dmoc_torque
#define GEVCUBIT12 ( 1 << 12) // cid_dmoc_critical_f
#define GEVCUBIT13 ( 1 << 13) // cid_dmoc_hv_status
#define GEVCUBIT14 ( 1 << 14) // cid_dmoc_hv_temps


/* Event status bit assignments (CoNtaCTor EVent ....) */
#define CNCTEVTIMER1 (1 << 0) // 1 = timer1 timed out: command/keep-alive
#define CNCTEVTIMER2 (1 << 1) // 1 = timer2 timed out: delay
#define CNCTEVTIMER3 (1 << 2) // 1 = timer3 timed out: uart RX/keep-alive
#define CNCTEVCACMD  (1 << 3) // 1 = CAN rcv: general purpose command
#define CNCTEVCANKA  (1 << 4) // 1 = CAN rcv: Keep-alive/command
#define CNCTEVCAPOL  (1 << 5) // 1 = CAN rcv: Poll
#define CNCTEVCMDRS  (1 << 6) // 1 = Command to reset
#define CNCTEVCMDCN  (1 << 7) // 1 = Command to connect
#define CNCTEVHV     (1 << 8) // 1 = New HV readings
#define CNCTEVADC    (1 << 9) // 1 = New ADC readings

/* Output status bit assignments */
#define CNCTOUT00K1  (1 << 0) // 1 = contactor #1 energized
#define CNCTOUT01K2  (1 << 1) // 1 = contactor #2 energized
#define CNCTOUT02X1  (1 << 2) // 1 = aux #1 closed
#define CNCTOUT03X2  (1 << 3) // 1 = aux #2 closed
#define CNCTOUT04EN  (1 << 4) // 1 = DMOC enable FET
#define CNCTOUT05KA  (1 << 5) // 1 = CAN msg queue: KA status
#define CNCTOUT06KAw (1 << 6) // 1 = contactor #1 energized & pwm'ed
#define CNCTOUT07KAw (1 << 7) // 1 = contactor #2 energized & pwm'ed
#define CNCTOUTUART3 (1 << 8) // 1 = uart3 rx with high voltage timer timed out

/* AUX contact pins. */
#define AUX1_GPIO_REG GPIOA
#define AUX1_GPIO_IN  GPIO_PIN_1
#define AUX2_GPIO_REG GPIOA
#define AUX2_GPIO_IN  GPIO_PIN_5

/* HV by-pass jumper pin. */
#define HVBYPASSPINPORT GPIOA
#define HVBYPASSPINPIN  GPIO_PIN_3

/* Command request bits assignments. */
#define CMDCONNECT (1 << 7) // 1 = Connect requested; 0 = Disconnect requested
#define CMDRESET   (1 << 6) // 1 = Reset fault requested; 0 = no command

/* Number of different CAN id msgs this function sends. */
# define NUMCANMSGS 6

/* High voltage readings */
//#define HVSCALEbits 16  // Scale factor HV

struct CNCNTHV
{
	struct IIRFILTERL iir; // Intermediate filter params
//   double dhv;            // Calibrated
	double dscale;         // volts/tick
	double dhvc;           // HV calibrated
	uint32_t hvcal;        // Calibrated, scaled volts/adc tick
	uint32_t hvc;          // HV as scaled volts
	uint16_t hv;           // Raw ADC reading as received from uart

};

/* Fault codes */
enum GEVCU_FAULTCODE
{
	NOFAULT,
	BATTERYLOW,
	GEVCU1_OFF_AUX1_ON,
	GEVCU2_OFF_AUX2_ON,
	GEVCU1_ON_AUX1_OFF,
	GEVCU2_ON_AUX2_OFF,
	GEVCU1_DOES_NOT_APPEAR_CLOSED,
   PRECHGVOLT_NOTREACHED,
	GEVCU1_CLOSED_VOLTSTOOBIG,
	GEVCU2_CLOSED_VOLTSTOOBIG,
	KEEP_ALIVE_TIMER_TIMEOUT,
	NO_UART3_HV_READINGS,
	HE_AUTO_ZERO_TOLERANCE_ERR,
};



/* Function command response payload codes. */
enum GEVCU_CMD_CODES
{
	ADCCTLLEVER,		// PC1 IN11 - CL reading
	ADCRAW5V,         // PC4 IN14 - 5V 
	ADCRAW12V,        // PC2 IN12 - +12 Raw
	ADCSPARE,			// PC5 IN15 - 5V ratiometric spare
	ADCINTERNALTEMP,  // IN17     - Internal temperature sensor
	ADCINTERNALVREF,  // IN18     - Internal voltage reference
	UARTWHV1,
	UARTWHV2,
	UARTWHV3,
	CAL5V,
	CAL12V,
	ADCRAWCUR1,       
	ADCRAWCUR2,       

};

/* CAN msg array index names. */
enum CANCMD_R
{
CID_KA_R,
CID_MSG1,
CID_MSG2,
CID_CMD_R,
CID_HB1,
CID_HB2,
};

/* Working struct for Gevcu function/task. */
// Prefixes: i = scaled integer, f = float
// Suffixes: k = timer ticks, t = milliseconds
struct GEVCUFUNCTION
{
   // Parameter loaded either by high-flash copy, or hard-coded subroutine
	struct GEVCULC lc; // Parameters for contactors, (lc = Local Copy)

	struct ADCFUNCTION* padc; // Pointer to ADC working struct

	/* Events status */
	uint32_t evstat;

	/* Output status */
	uint32_t outstat;
	uint32_t outstat_prev;

	/* Current fault code */
	enum GEVCU_FAULTCODE faultcode;
	enum GEVCU_FAULTCODE faultcode_prev;


	uint32_t ka_k;       // GevcuTask polling timer.
	uint32_t keepalive_k;// keep-alive timeout (timeout delay ticks)
	uint32_t hbct_k;		// Heartbeat ct: ticks between sending msgs hv1:cur1

	uint32_t ihv1;       // Latest reading: battery string at contactor #1
	uint32_t ihv2;       // Latest reading: DMOC side of contactor #1
	uint32_t ihv3;       // Latest reading: Pre-charge R, (if two contactor config)
	int32_t hv1mhv2;     // Voltage across contactor #1 from latest readings

	uint32_t statusbits;
	uint32_t statusbits_prev;

	/* Setup serial receive for uart (HV sensing) */
	struct SERIALRCVBCB* prbcb3;	// usart3

	TimerHandle_t swtimer1; // Software timer1: command/keep-alive
	TimerHandle_t swtimer2; // Software timer2: multiple purpose delay
	TimerHandle_t swtimer3; // Software timer3: uart RX/keep-alive


	/* Pointers to incoming CAN msg mailboxes. */
	struct MAILBOXCAN* pmbx_cid_cntctr_keepalive_r; // CANID_CMD_CNTCTRKAR: U8_VAR: Contactor1: R KeepAlive response to poll
	struct MAILBOXCAN* pmbx_cid_dmoc_actualtorq; // CANID_DMOC_ACTUALTORQ:I16,   DMOC: Actual Torque: payload-30000
	struct MAILBOXCAN* pmbx_cid_dmoc_speed;      // CANID_DMOC_SPEED:     I16_X6,DMOC: Actual Speed (rpm?)
	struct MAILBOXCAN* pmbx_cid_dmoc_dqvoltamp;  // CANID_DMOC_DQVOLTAMP: I16_I16_I16_I16','DMOC: D volt:amp, Q volt:amp
	struct MAILBOXCAN* pmbx_cid_dmoc_torque;     // CANID_DMOC_TORQUE:    I16_I16,'DMOC: Torque,-(Torque-30000)
	struct MAILBOXCAN* pmbx_cid_dmoc_critical_f; // CANID_DMOC_TORQUE:    NONE',   'DMOC: Critical Fault: payload = DEADB0FF
	struct MAILBOXCAN* pmbx_cid_dmoc_hv_status;  // CANID_DMOC_HV_STATUS: I16_I16_X6,'DMOC: HV volts:amps, status
	struct MAILBOXCAN* pmbx_cid_dmoc_hv_temps;   // CANID_DMOC_HV_TEMPS:  U8_U8_U8,  'DMOC: Temperature:rotor,invert,stator
	struct MAILBOXCAN* pmbx_cid_gps_sync; // CANID_HB_TIMESYNC:  U8 : GPS_1: U8 GPS time sync distribution msg-GPS time sync msg

	uint8_t state;      // Gevcu main state
	uint8_t substateC;  // State within CONNECTING (0-15)
	uint8_t substateX;  // spare substate (0-15)

	/* CAN msgs */
	struct CANTXQMSG canmsg[NUMCANMSGS];
};

/* *************************************************************************/
osThreadId xGevcuTaskCreate(uint32_t taskpriority);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: GevcuTaskHandle
 * *************************************************************************/
void StartGevcuTask(void const * argument);
/*	@brief	: Task startup
 * *************************************************************************/

extern struct GEVCUFUNCTION gevcufunction;
extern osThreadId GevcuTaskHandle;

#endif

