/******************************************************************************
* File Name          : SwitchTask.c
* Date First Issued  : 02/042020
* Description        : Updating switches from spi shift register
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"

#include "SwitchTask.h"
#include "spiserialparallel.h"
#include "SpiOutTask.h"
#include "shiftregbits.h"
#include "GevcuTask.h"
#include "morse.h"

/* Theoretical: 10 spi cycles per millisecond, and each has a switch change
   that puts a uint16_t word on the queue. Obviously, something wrong with
   shift-register and switches. 128 words would be about 12.8 ms before there
   would be a queue overrun. */
#define SWITCHQSIZE 128

osThreadId SwitchTaskHandle    = NULL;
osMessageQId SwitchTaskQHandle = NULL;



enum SWPAIRSTATE
{
	SWP_UNDEFINED, /* 0 */
	SWP_SAFE,      /* 1 */
	SWP_ACTIVE,    /* 2 */
	SWP_DEBOUNCE   /* 3 */
};

/*
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
#define CP_BRAKE   (1 <<  8)  // Brake        P8-11 IN  8 H GevcuTask.h
*/

/* Logical result of SW_SAFE and SW_ACTIVE */
uint8_t sw_active;  // 1 = active; 0 = safe
struct SWPAIR swpair_safeactive;

/* Local copy (possibly delayed slightly) of spi read word. */
uint16_t spilocal;

#define SWPAIR_DB_SAFEACTIVE 1 // Debounce

uint32_t spitickctr; // Running count of spi time ticks (mostly for debugging)
uint32_t swxctr;

/*******************************************************************************
Inline assembly.  Hacked from code at the following link--
http://balau82.wordpress.com/2011/05/17/inline-assembly-instructions-in-gcc/
*******************************************************************************/
static inline __attribute__((always_inline)) unsigned int arm_clz(unsigned int v) 
{
  unsigned int d;
  asm ("CLZ %[Rd], %[Rm]" : [Rd] "=r" (d) : [Rm] "r" (v));
  return d;
}

/* *************************************************************************
 * static void swinit(void);
 *	@brief	: 
 * *************************************************************************/
static void sw_init(void)
{
// TODO Anything?
	return;
}
/* *************************************************************************
 * static void spitick(void);
 *	@brief	: Debounce timing
 * *************************************************************************/
static void spitick(void)
{
	spitickctr += 1; // This will be replaced
	return;
}
/* *************************************************************************
 * static void swchanges(uint16_t spixor);
 *	@brief	: 
 * *************************************************************************/
static void swchanges(uint16_t spixor)
{
	uint16_t swb;

/* Not so elegant handling. */
	/* SAFE/ACTIVE logic. */
	// Any changes to this switch pair?	
	if ((spixor & (SW_SAFE | SW_ACTIVE)) != 0) 
	{ // Here, yes.
		// New readings of both switches
swxctr += 1;
		swb = spilocal & (SW_SAFE | SW_ACTIVE);
		switch (swpair_safeactive.state)
		{
		case SWP_UNDEFINED:
// NOTE: Remember switch making contact has the bit = 0
				// Look for SAFE on & ACTIVE off
				if (swb != SW_ACTIVE) break; 
				swpair_safeactive.state = SWP_SAFE;

		case SWP_SAFE: // We are in safe sw state
			if (!(swb == SW_SAFE)) // Not 10? 
				break; // break: 00 01 11  remain safe state

			// Here, sws are 10: safe sw is open (1) active closed (0)
			swpair_safeactive.state = SWP_ACTIVE;
			// Notify GevcuTask switches changed to ACTIVE
			xTaskNotify(GevcuTaskHandle, GEVCUBIT01, eSetBits);

			case SWP_ACTIVE: // In active state
			if (swb == SW_SAFE)
				break; // break: 10 remain in active state

			// Here, 11,10,00, i.e. not (ACTIVE closed AND safe open)	
			swpair_safeactive.state = SWP_SAFE;
		
			// Notify GevcuTask that switches changed to SAFE
			xTaskNotify(GevcuTaskHandle, GEVCUBIT02, eSetBits);
			break;
		}
	}
}
/* *************************************************************************
 * void StartSwitchTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartSwitchTask(void* argument)
{
	BaseType_t qret;
	uint16_t spixor; // spi read-in difference

   /* Infinite loop */
   for(;;)
   {
		qret = xQueueReceive( SwitchTaskQHandle,&spixor,portMAX_DELAY);
		if (qret == pdPASS)
		{ // Here we have a value from the queue
			if (spixor == 0)
			{ // Here, spi interrupt time tick
				spitick();
			}
			else
			{ // Deal with changes
				spilocal = (spilocal ^ spixor); // Update local copy
				swchanges(spixor);
			}
		}
   }
}
/* *************************************************************************
 * osThreadId xSwitchTaskCreate(uint32_t taskpriority);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @return	: SwitchTaskHandle, or NULL if queue or task creation failed
 * *************************************************************************/
osThreadId xSwitchTaskCreate(uint32_t taskpriority)
{
	BaseType_t ret = xTaskCreate(&StartSwitchTask, "SwitchTask",\
     64, NULL, taskpriority, &SwitchTaskHandle);
	if (ret != pdPASS) return NULL;

	SwitchTaskQHandle = xQueueCreate(SWITCHQSIZE, sizeof(uint16_t) );
	if (SwitchTaskQHandle == NULL) return NULL;

	sw_init();

	return SwitchTaskHandle;
}


