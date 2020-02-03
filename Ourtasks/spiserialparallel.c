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

uint32_t spispctr; // Cycle counter

osThreadId spinotify = NULL; // Task to notify when read work changes
uint32_t spinotebits = 0;    // Task notify: bits that changed

static SPI_HandleTypeDef *pspix;

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
	spispctr += 1;

/* NOTE: A delay is needed between the set and reset of the 
   I/O pin. Otherwise it is too fast for the pin to follow.
   The following has statements that could be in the 'if',
   outside the 'if' so that they add to the delay when the 'if'
   is not taken.
*/
	/* Set the bits that changed in the notification word. */
	spinotebits = (spisp_rd_prev[0].u16 ^ spisp_rd[0].u16);
	
	/* If any bits change, notify a task. */
	if (spinotebits == 0)
	{ // Input (read) word has changed. Update and do something

		/* (We are under interrupt) trigger a task to deal with this. */
		if (spinotify != NULL)
		{ // Here, a task handle has been supplied
			xTaskNotifyFromISR(spinotify,spinotebits,eSetBits,&hptw); 
			portYIELD_FROM_ISR(hptw);
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
