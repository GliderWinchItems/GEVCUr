/******************************************************************************
* File Name          : LcdTask.c
* Date First Issued  : 04/05/2020,06/12/2020
* Description        : LCD display 
*******************************************************************************/
/*
06/12/2020 - Revised for circular line buffers
*/

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "stm32f4xx_hal.h"
#include "LcdTask.h"
#include "lcd_hd44780_i2c.h"
#include "morse.h"
#include "LcdmsgsTask.h"
#include "LcdmsgsetTask.h"
#include "yprintf.h"

struct LCDI2C_UNIT* punitd4x20 = NULL;
//struct LCDI2C_UNIT* punitd4x16 = NULL;
//struct LCDI2C_UNIT* punitd2x16 = NULL;

volatile uint8_t LcdTaskflag = 0;

extern I2C_HandleTypeDef hi2c1;

enum LCD_STATE
{
	LCD_IDLE,
	LCD_SETRC,
	LCD_ROW,
	LCD_COL,
	LCD_CHR
};

/* LCD Display */
uint8_t lcdcontext; // Context: Flag bits for LCD line msg priority

TaskHandle_t LcdTaskHandle = NULL;

/* Queue */
#define QUEUESIZE 32	// Total size of bcb's other tasks can queue up
QueueHandle_t LcdTaskQHandle;

/* Circular buffer of lcd line buffers. */
static struct LCDTASK_LINEBUF* padd;
static struct LCDTASK_LINEBUF* pend;
static struct LCDTASK_LINEBUF* pbegin;
static struct LCDTASK_LINEBUF* ptaken;

/* Linked list head. */
static struct LCDI2C_UNIT* phdunit = NULL;   // Units instantiated; last = NULL
/* *************************************************************************
 * struct LCDI2C_UNIT* xLcdTaskcreateunit(I2C_HandleTypeDef* phi2c, 
 *    uint8_t address, 
 *    uint8_t numrow, 
 *    uint8_t numcol);
 *	@brief	: Instantiate a LCD unit on I2C peripheral and address
 * @param	: phi2c = pointer to I2C handle
 * @param	: address = I2C bus address
 * @param	: numrow = number of LCD rows
 * @param	: numcol = number of LCD columns
 * @return	: NULL = fail, otherwise pointer to unit on linked list
 * *************************************************************************/
struct LCDI2C_UNIT* xLcdTaskcreateunit(I2C_HandleTypeDef* phi2c, 
    uint8_t address, 
    uint8_t numrow, 
    uint8_t numcol)
{
	struct LCDI2C_UNIT* punit;
	struct LCDI2C_UNIT* ptmp = NULL;

taskENTER_CRITICAL();
	/* Check if this I2C bus & address (i.e. unit) is already present. */
	punit = phdunit; // Get pointer to first item on list
	if (punit == NULL)
	{ // Linked list is empty, so add first unit
		punit = (struct LCDI2C_UNIT*)calloc(1, sizeof(struct LCDI2C_UNIT));
		if (punit == NULL) morse_trap(230);
		phdunit = punit;  // Head points to first entry
//morse_trap(44);
	}
	else
	{ // Here, one or more on list.
		/* Search list for this I2C-address */
		while (punit != NULL)
		{
			if ((punit->phi2c == phi2c) && (punit->address == address))
			{ // Here this I2C-address is already on list.
				morse_trap(231); // ### ERROR: Duplicate ###
			}
			ptmp = punit;
			punit = punit->pnext;
		}
		/* Here, one or more is on list, but not this one. */
		punit = (struct LCDI2C_UNIT*)calloc(1, sizeof(struct LCDI2C_UNIT));
		if (punit == NULL) morse_trap(236);
		ptmp->pnext    = punit;  // Previous last entry points this new entry
	}

	/* Populate the control block for this unit. */
	punit->phi2c   = phi2c;   // HAL I2C Handle for this bus
	punit->address = address; // I2C bus address (not shifted)

	/* The remainder of LCDPARAMS will be initialized in the lcdInit() below. */
	punit->lcdparams.hi2c     = phi2c;
	punit->lcdparams.numrows  = numrow;  // number of LCD rows
	punit->lcdparams.numcols  = numcol;  // number of LCD columns
	punit->lcdparams.address  = address << 1; // Shifted address
   	punit->lcdparams.lines    = numrow;
   	punit->lcdparams.columns  = numcol;

	punit->state = LCD_IDLE; // Start off in idle (after final intialization)

taskEXIT_CRITICAL();

	/* Complete the initialization the LCD unit */ 
	struct LCDPARAMS* tmp = lcdInit(punit);
	if (tmp == NULL) morse_trap(237);

	return punit;
}
/* *************************************************************************
 * void StartLcdTask(void const * argument);
 *	@brief	: Task startup
 * *************************************************************************/
void StartLcdTask(void* argument)
{
	BaseType_t Qret = 1;	// queue receive return
	struct LCDTASK_LINEBUF* plb; // Copied item from queue
	struct LCDPARAMS* p1;
	struct LCDI2C_UNIT* ptmp; // Ptr to LCD unit control block

  /* Instantiate each LCD I2C display unit. 
     Arguments:
       I2C bus handle
       Address of unit on I2C bus
       Number of rows (lines)
       Number of columns                    */
  punitd4x20 = xLcdTaskcreateunit(&hi2c1,0x27,4,20);
  if (punitd4x20 == NULL) morse_trap(227);

  osThreadId retThrd= xLcdmsgsetTaskCreate(0, 32);
  if (retThrd == NULL) morse_trap(125);

    /* Let Tasks know that we are ready accept msgs. */
    LcdTaskflag = 1;

  /* Infinite loop */
  for(;;)
  {
if (LcdTaskQHandle == NULL) morse_trap(229); // JIC debugging

	Qret = xQueueReceive( LcdTaskQHandle,&plb,portMAX_DELAY);
	if (Qret == pdPASS)
	{ // Here, OK item from queue
//HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_14); // Red
		ptmp = plb->punit;  // Get pointer to LCD unit control block
		//if (ptmp != NULL) // JIC
		if (ptmp == NULL) morse_trap(2211);
		{
			// Get LCDPARAMS pointer from lcd unit block
			p1 = &ptmp->lcdparams;
    
    		// Set cursor row/column for this unit
			lcdSetCursorPosition(p1,plb->colreq,plb->linereq);

			// Send the glorious text of liberation! (Or, at least some ASCII)
  			lcdPrintStr(p1,&plb->buf[0], plb->size);

  			// Update for overrun protection in 'printf and 'puts 
  			ptaken = plb;
		}
	}
  }
}
/* *************************************************************************
 * osThreadId xLcdTaskCreate(uint32_t taskpriority, uint16_t numbuf);
 * @brief	: Create task; task handle created is global for all to enjoy!
 * @param	: taskpriority = Task priority (just as it says!)
 * @param 	: numbuf = number of lcd line buffers
 * @return	: LcdTaskHandle
 * *************************************************************************/
osThreadId xLcdTaskCreate(uint32_t taskpriority, uint16_t numbuf)
{

	/* Setup a circular buffer of lcd line buffers. */
taskENTER_CRITICAL();
	pbegin = (struct LCDTASK_LINEBUF*)calloc(numbuf,sizeof(struct LCDTASK_LINEBUF));
	if (pbegin == NULL) morse_trap(372);
	padd   = pbegin;
	ptaken = pbegin;
	pend   = pbegin + numbuf;
taskEXIT_CRITICAL();

/* xTaskCreate 
  	BaseType_t xTaskCreate( TaskFunction_t pvTaskCode,
  	const char * const pcName,
  	unsigned short usStackDepth,
  	void *pvParameters,
  	UBaseType_t uxPriority,
  	TaskHandle_t *pxCreatedTask );   
 */
	BaseType_t ret = xTaskCreate(&StartLcdTask,"LcdI2CTask",96,NULL,taskpriority,&LcdTaskHandle);
	if (ret != pdPASS) morse_trap(35);//return NULL;

	/* Create a queue of buffer pointers that point into the circular buffer (see above) */
	LcdTaskQHandle = xQueueCreate(numbuf, sizeof(struct LCDTASK_LINEBUF*) );
	if (LcdTaskQHandle == NULL) morse_trap(370);//return NULL;

	if (vsnprintfSemaphoreHandle == NULL)
		vsnprintfSemaphoreHandle = xSemaphoreCreateMutex(); // Semaphore for 'printf'
	if (vsnprintfSemaphoreHandle == NULL) morse_trap(371);

	return LcdTaskHandle;
}
/* **************************************************************************************
* NOTE: This is called from LcdmsgssetTask.c
 * int lcdi2cprintf(struct LCDI2C_UNIT** pplb, int row, int col, const char *fmt, ...);
 * @brief	: 'printf' for uarts
 * @param	: pblb = pointer to pointer to line buff struct
 * @param   : row = row number (0-n) to position cursor
 * @param   : col = column number (0-n) to position cursor
 * @param	: format = usual printf format
 * @param	: ... = usual printf arguments
 * @return	: Number of chars "printed"
 * ************************************************************************************** */
// NOTE: This is called from LcdmsgssetTask.c
int lcdi2cprintf(struct LCDI2C_UNIT** pplb, int row, int col, const char *fmt, ...)
{
	va_list argp;
	int8_t tmp;
	uint8_t ct;


	padd->punit   = *pplb; // LCD unit block pointer
	padd->linereq = row;  // Line (e.g. 0 - 3)
	padd->colreq  = col;  // Column (e.g. 0 - 18)

	/* Block if vsnprintf is being uses by someone else. */
	BaseType_t ret = xSemaphoreTake( vsnprintfSemaphoreHandle, portMAX_DELAY );
	if (ret == pdFAIL) morse_trap(382); // JIC debugging

	/* Construct line of data.  Stop filling buffer if it is full. */
	va_start(argp, fmt);
	padd->size = vsnprintf((char*)&padd->buf[0], LCDLINEMAX+1, fmt, argp);
	va_end(argp);

	/* Release semaphore controlling vsnprintf. */
	xSemaphoreGive( vsnprintfSemaphoreHandle );

	if (padd->size < 0) return 0; // vsnprintf error encountered

	/* If vsnprintf truncated the writing, limit the count. */
	if (padd->size > (LCDLINEMAX+1)) padd->size = LCDLINEMAX;

	/* Place pointer to Buffer Control Block pointer on queue to LcdTask */
	//   When queue is full, wait a limited amount of time for LCDTask to
	// complete sending a line.
#define QDELAY1  10	// Number of polls before quitting
#define OSDELAY1 15 // Number of os ticks for each poll delay
	ct = 0;
	while ((xQueueSendToBack(LcdTaskQHandle, &padd, OSDELAY1) == pdFAIL) && (ct++ < QDELAY1) )
	if (ct >= QDELAY1) morse_trap(384);

	/* Advance pointer to next available buffer. */
	tmp = padd->size;
	padd += 1; if (padd == pend) padd = pbegin;

	/* Overrun stall. Wait for open buffer position. */
	// This prevents the *NEXT* entry into this routine from overrunning
#define QDELAY2  10	// Number of polls before quitting
#define OSDELAY2 15 // Number of os ticks for each poll delay
	ct = 0;
	while ((padd == ptaken) && (ct++ < QDELAY2)) osDelay(OSDELAY2);
	if (ct >= QDELAY2) morse_trap(385);

	return tmp;
}
/* **************************************************************************************
 * NOTE: This is called from LcdmsgssetTask
 * int lcdi2cputs(struct LCDI2C_UNIT** pplb, int row, int col, char* pchr);
 * @brief	: Send zero terminated string to SerialTaskSend
 * @param	: pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
 * @param   : row = row number (0-n) to position cursor
 * @param   : col = column number (0-n) to position cursor
 * @return	: Number of chars sent
 * ************************************************************************************** */
// NOTE: This is called from LcdmsgssetTask.c
int lcdi2cputs(struct LCDI2C_UNIT** pplb, int row, int col, char* pchr)
{
	int8_t tmp;
	uint8_t ct;

	int sz = strlen(pchr); // Check length of input string
	if (sz == 0) return 0;

	padd->punit   = *pplb; // LCD unit block pointer
	padd->linereq = row;  // Line (e.g. 0 - 3)
	padd->colreq  = col;  // Column (e.g. 0 - 18)

	strncpy((char*)&padd->buf[0],pchr,LCDLINEMAX+1);	// Copy and limit size.

	/* Set size sent. */
	if (sz >= LCDLINEMAX)	// Did strcpy truncate?
		padd->size = LCDLINEMAX;	// Yes
	else
		padd->size = sz;	// No

	/* Place pointer to Buffer Control Block pointer on queue to LcdTask */
	//   When queue is full, wait a limited amount of time for LCDTask to
	// complete sending a line.
#define QDELAY1  10	// Number of polls before quitting
#define OSDELAY1 15 // Number of os ticks for each poll delay
	ct = 0;
	while ((xQueueSendToBack(LcdTaskQHandle, &padd, OSDELAY1) == pdFAIL) && (ct++ < QDELAY1) )
	if (ct >= QDELAY1) morse_trap(384);

	/* Advance pointer to next available buffer. */
	tmp = padd->size;
	padd += 1; if (padd == pend) padd = pbegin;

	/* Overrun stall. Wait for open buffer position. */
	// This prevents the *NEXT* entry into this routine from overrunning
	ct = 0;
	while ((padd == ptaken) && (ct++ < QDELAY2)) osDelay(OSDELAY2);
	if (ct >= QDELAY2) morse_trap(385);

	return tmp; 
}