/******************************************************************************
* File Name          : spiserialparallel.c
* Date First Issued  : 10/13/2019
* Description        : SPI serial<->parallel extension
*******************************************************************************/

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "stm32f4xx_hal.h"
#include "spiserialparallel.h"
#include "morse.h"
#include "SwitchTask.h"

/* Count number of spi interrupts with no switches changing. 
   and place zero changes on queue to SwitchTask.

   This is a just-in-case something got lost. */
#define SPINCNOTIFYCTR 500	// Number of spi interrupts between notifies	
static uint16_t spinotifyctr; // Count interrupts, reset to zero

/**
  * @brief  Transmit and Receive an amount of data in non-blocking mode with Interrupt.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @param  pTxData pointer to transmission data buffer
  * @param  pRxData pointer to reception data buffer
  * @param  Size amount of data to be sent and received
  * @retval HAL status
  */
//HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size)

/* Input and Output mirror of hw registers. */
union SPISP spisp_rd[SPISERIALPARALLELSIZE/2];
union SPISP spisp_wr[SPISERIALPARALLELSIZE/2];

/* Previous 'read' word, used to check for switch changes. */
static union SPISP spisp_rd_prev[SPISERIALPARALLELSIZE/2];

uint32_t spispctr; // spi interrupt counter for debugging.

static SPI_HandleTypeDef *pspix;

static uint16_t spinotebits;

/* *************************************************************************
 * HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi);
 *	@brief	: 
 * @return	: success = HAL_OK
 * *************************************************************************/
static volatile int dly;
HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi)
{
	pspix = phspi;	// Save pointer to spi contol block

	/* Enable output shift register pins. */

	HAL_GPIO_WritePin(GPIOE,GPIO_PIN_6,GPIO_PIN_RESET); // Not OE pins
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);

	/* Start SPI running. */
	return HAL_SPI_TransmitReceive_IT(phspi, 
		&spisp_wr[0].u8[0], 
		&spisp_rd[0].u8[0], 
		SPISERIALPARALLELSIZE);
}

/**
  * @brief Tx and Rx Transfer completed callback.
  * @param  hspi pointer to a SPI_HandleTypeDef structure that contains
  *               the configuration information for SPI module.
  * @retval None
  */
// __weak void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
	BaseType_t hptw = pdFALSE;
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);

/* NOTE: A delay is needed between the set and reset of the 
   I/O pin. Otherwise it is too fast for the pin to follow.
   The following has statements that could be in the 'if',
   outside the 'if' so that they add to the delay when the 'if'
   is not taken.
*/
	spispctr += 1;     // Debugging ctr
	spinotifyctr += 1; // SwitchTask update timer ctr

	/* Set the bits that changed in the notification word. */
	spinotebits = (spisp_rd_prev[0].u16 ^ spisp_rd[0].u16);
	
	/* If any bits change, notify a task. */
	if (spinotebits != 0)
	{ // Input (read) word has changed. Load queue to notify SwitchTask

		/* (We are under interrupt) trigger a task to deal with this. */
		if (SwitchTaskQHandle != NULL)
		{ // Here, a task handle has been supplied
			xQueueSendToBackFromISR(SwitchTaskQHandle,&spinotebits,&hptw);
		}
	}
	else
	{ // Here no changes in switches. Queue a "zero" periodically
		if (spinotifyctr >= SPINCNOTIFYCTR)
		{ // Here, we have gone approximately 100ms without a switch change
			spinotifyctr  = 0; // Reset timing counter
			/* (We are under interrupt) trigger a task to deal with this. */
			if (SwitchTaskQHandle != NULL)
			{ // Here, a task handle has been supplied
				xQueueSendToBackFromISR(SwitchTaskQHandle,&spinotebits,&hptw);
			}
		}
	}
	spisp_rd_prev[0].u16 = spisp_rd[0].u16; // Update previous
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);

	/* Re-trigger another xmit/rcv cycle. */
	HAL_SPI_TransmitReceive_IT(pspix, 
		&spisp_wr[0].u8[0], 
		&spisp_rd[0].u8[0], 
		SPISERIALPARALLELSIZE);

	return;
}
