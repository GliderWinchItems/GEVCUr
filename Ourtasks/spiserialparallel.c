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
uint32_t spispctr; // Cycle counter

static SPI_HandleTypeDef *pspix;

/* *************************************************************************
 * HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi);
 *	@brief	: 
 * @return	: success = HAL_OK
 * *************************************************************************/
HAL_StatusTypeDef spiserialparallel_init(SPI_HandleTypeDef* phspi)
{
	pspix = phspi;	// Save pointer to spi contol block

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
		spispctr += 1;

HAL_SPI_TransmitReceive_IT(pspix, 
		&spisp_wr[0].u8[0], 
		&spisp_rd[0].u8[0], 
		SPISERIALPARALLELSIZE);

	return;
}
