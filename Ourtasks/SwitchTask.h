/******************************************************************************
* File Name          : SwitchTask.h
* Date First Issued  : 02/042020
* Description        : Updating switches from spi shift register
*******************************************************************************/

#ifndef __SWITCHTASK
#define __SWITCHTASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "spiserialparallel.h"

#define NUMSWS (SPISERIALPARALLELSIZE*8)	// Number of possible switches

/* Switches have pull-ups so open = 1. */
#define SW_CLOSED 0
#define SW_OPEN   1
#define SW_ERROR  2 // Logically an error

/* PB Debounce modes. */
#define SWMODE_NOW      0 // Immediate, w debounce minimum 
#define SWMODE_WAIT     1 // Wait until debounce ends
#define SWMODE2_RS      2 // Switch pair R-S flip flop mode
#define SWMODE2_RS_00   3 // Switch pair 00 debounced
#define SWMODE2_RS_11   4 // Switch pair 11 debounced
#define SWMODE2_RS_0011 5 // Switch pair 00 & 11 debounced

/* Type of switch. */
#define SWTYPE_PB   0 // ON/OFF (Pushbutton), e.g. SPST-NO
#define SWTYPE_PAIR 1 // Switch pairs, e.g. SPDT.

/* Each on/off switch used has one of these added to a linked list. */
struct SWITCHPTR
{
	// Linked list pointers
	struct SWITCHPTR* pnext; // Points to next sw; Last is NULL
	struct SWITCHPTR* pdbnx; // Points to next active debounce sw

   // Task notification
	osThreadId tskhandle; // Task handle (NULL = use calling task)
	uint32_t notebit;     // notification bit; 0 = no notification

	// Switch bit position(s). 
	uint32_t switchbit;   // Switch bit position in spi read word
	uint32_t switchbit1;  // If not null: 2nd for switch pair
	uint8_t type;         // 
		// 0 = Pushbutton; Not 0 = switch pair
		// 1 = switch pair 1st sw
		// 2 = switch pair 2nd sw

	// Switch status (w pullups): SW_OPEN (1), SW_CLOSED (0)
	uint8_t on;           // Latest spi reading
	uint8_t db_on;        // Debounced representation

	// Debouncing 
	uint8_t db_mode;        // Debounce type: SWMODE_NOW, SWMODE_WAIT
	 int8_t db_ctr;         // Countdown working counter
	 int8_t db_dur_closing; // spi ticks for countdown
	 int8_t db_dur_opening; // spi ticks for countdown

	uint8_t state;          // Switch state	
};	

/* *************************************************************************/
 struct SWITCHPTR* switch_pb_add(osThreadId tskhandle, uint32_t notebit, 
    uint32_t switchbit,
    uint32_t switchbit1,
    uint8_t type,
    uint8_t db_mode, 
    uint8_t dur_closing,
    uint8_t dur_opening);
/*
 *	@brief	: Add a single on/off (e.g. pushbutton) switch on a linked list
 * @param	: tskhandle = Task handle; NULL to use current task; 
 * @param	: notebit = notification bit; 0 = no notification
 * @param	: switchbit  = bit position in spi read word for this switch
 * @param	: switchbit1 = bit position for 2nd switch if a switch pair
 * @param	: type: 0 = on/off (pushbutton), 1 = switch pair
 * @param	: db_mode = debounce mode: 0 = immediate; 1 = wait debounce
 * @param	: dur_closing = number of spi time tick counts for debounce
 * @param	: dur_opening = number of spi time tick counts for debounce
 * @return	: Pointer to struct for this switch
 * *************************************************************************/
osThreadId xSwitchTaskCreate(uint32_t taskpriority);
/* @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: SwitchTaskHandle, or NULL if queue or task creation failed
 * *************************************************************************/
void StartSwitchTask(void* argument);
/*	@brief	: Task startup
 * *************************************************************************/

extern osMessageQId SwitchTaskQHandle;
extern osThreadId   SwitchTaskHandle;

extern struct SWITCHPTR swpair_safeactive;

extern struct SWITCHPTR* pb_reversetorq;

#endif

