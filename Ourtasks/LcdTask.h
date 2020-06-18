/******************************************************************************
* File Name          : LcdTask.h
* Date First Issued  : 04/05/2020,06/12/2020
* Description        : LCD display 
*******************************************************************************/

#ifndef __LCDTASK
#define __LCDTASK

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "semphr.h"

#define LCDX_CLIB  0x01  // 1 = CL Calibration complete
#define LCDX_CNTR  0x02  // 1 = Contactor is in fault condition
#define LCDX_DMOC  0x04  // 1 = DMOC is in fault condition


#define LCDLINEMAX 20   // Max length of LCD line
/* I2C Line buffer */
struct LCDTASK_LINEBUF
{
	struct LCDI2C_UNIT* punit;  // Point to linked list entry for this I2C:address unit
	 int8_t size;               // Number of bytes to be sent
  uint8_t linereq;            // Line number requested(0 - n)
  uint8_t colreq;             // Column number requested(0 - n)
  uint8_t buf[LCDLINEMAX+1];  // Data to display (plus zero terminator)
};

struct LCDPARAMS 
{
    I2C_HandleTypeDef * hi2c;  // I2C Struct
    uint8_t lines;             // Lines of the display
    uint8_t columns;           // Columns
    uint8_t address;           // I2C address =>shifted<= left by 1
    uint8_t backlight;         // Backlight
    uint8_t modeBits;          // Display on/off control bits
    uint8_t entryBits;         // Entry mode set bits
 	  uint8_t lcdCommandBuffer[8];
    uint8_t numrows;  // Number of rows (lines) for this LCD unit
    uint8_t numcols;  // Number of columns for this LCD unit
};

/* Linked list of entries for each I2C:address (i.e. LCD units) */
struct LCDI2C_UNIT
{
	struct LCDI2C_UNIT* pnext; // Next bus unit on this I2C bus
	I2C_HandleTypeDef* phi2c;  // HAL I2C Handle for this bus
	struct LCDPARAMS lcdparams;
	TickType_t untiltickct; // Tickcount for the end of a delay
	uint8_t address;  // =>Not-shifted<= address
	uint8_t state;    // State machine
};

/* *************************************************************************/
 void StartLcdTask(void* argument);
/*	@brief	: Task startup
 * *************************************************************************/
osThreadId xLcdTaskCreate(uint32_t taskpriority, uint16_t numbuf);
/* @brief : Create task; task handle created is global for all to enjoy!
 * @param : taskpriority = Task priority (just as it says!)
 * @param   : numbuf = number of lcd line buffers
 * @return  : LcdTaskHandle
 * *************************************************************************/
struct LCDI2C_UNIT* xLcdTaskcreateunit(I2C_HandleTypeDef* phi2c, 
    uint8_t address,
    uint8_t numrow,
    uint8_t numcol);
/*	@brief	: Instantiate a LCD unit on I2C peripheral and address
 * @param	: phi2c = pointer to I2C handle
 * @param	: address = I2C bus address
 * @param	: numrow = number of LCD rows
 * @param	: numcol = number of LCD columns
 * @return	: NULL = fail, otherwise pointer to unit on linked list
  * *************************************************************************/
 // NOTE: This is called from LcdmsgssetTask.c
 int lcdi2cprintf(struct LCDI2C_UNIT** pplb, int row, int col, const char *fmt, ...);
 /* @brief : 'printf' for uarts
  * @param : pblb = pointer to pointer to line buff struct
  * @param   : row = row number (0-n) to position cursor
  * @param   : col = column number (0-n) to position cursor
  * @param : format = usual printf format
  * @param : ... = usual printf arguments
  * @return  : Number of chars "printed"
 * ************************************************************************************** */
// * NOTE: This is called from LcdmsgssetTask
 int lcdi2cputs(struct LCDI2C_UNIT** pplb, int row, int col, char* pchr);
 /* @brief : Send zero terminated string to SerialTaskSend
  * @param : pbcb = pointer to pointer to stuct with uart pointers and buffer parameters
  * @param   : row = row number (0-n) to position cursor
  * @param   : col = column number (0-n) to position cursor
  * @return  : Number of chars sent
  * ************************************************************************************* */

extern QueueHandle_t  LcdTaskQHandle;
extern TaskHandle_t   LcdTaskHandle;

extern struct LCDI2C_UNIT* punitd4x20;
extern struct LCDI2C_UNIT* punitd4x16;
extern struct LCDI2C_UNIT* punitd2x16;

extern uint8_t lcdcontext; // Context: Flag bits for LCD line msg priority

extern volatile uint8_t LcdTaskflag;

#endif
