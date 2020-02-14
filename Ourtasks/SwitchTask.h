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
#define SW_OPEN   1
#define SW_CLOSED 0

/* PB Debounce modes. */
#define SW_NOW     0 // Immediate, w debounce minimum 
#define SW_WAITDB  1 // Wait until debounce ends

/* Each on/off switch used has one of these added to a linked list. */
struct SWITCHPTR
{
	struct SWITCHPTR* pnext; // Points to next sw; Last is NULL
	struct SWITCHPTR* pdbnx; // Points to next active debounce sw
	osThreadId tskhandle; // Task handle (NULL = use calling task)
	uint32_t notebit;     // notification bit; 0 = no notification
	uint32_t switchbit;   // Switch bit position in spi read word
	uint8_t on;           // 0 = sw is off, 1 = sw is on
	uint8_t db_on;        // sw representation (debounce logic executed)
	uint8_t db_mode;      // Debounce type 0 = on 1st; 1 = wait 
	uint8_t state;        // Switch state
	 int8_t db_ctr;         // Debounce: counter
	 int8_t db_dur_closing; // Debounce: spi ticks for debouncing
	 int8_t db_dur_opening; // Debounce: spi ticks for debouncing
};	
/* *************************************************************************/
struct SWITCHPTR* switch_pb_add(osThreadId tskhandle, uint32_t notebit, 
	 uint32_t switchbit,
	 uint8_t db_mode, 
	 uint8_t dur_closing,
	 uint8_t dur_opening);
/*
 *	@brief	: Add a single on/off (e.g. pushbutton) switch on a linked list
 * @param	: tskhandle = Task handle; NULL to use current task; 
 * @param	: notebit = notification bit; NULL = no notification
 * @param	: switchbit = bit position in spi read word for this switch
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

