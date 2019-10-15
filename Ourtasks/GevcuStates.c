/******************************************************************************
* File Name          : GevcuStates.c
* Date First Issued  : 07/01/2019
* Description        : States in Gevcu function w STM32CubeMX w FreeRTOS
*******************************************************************************/

#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "ADCTask.h"
#include "adctask.h"

#include "GevcuStates.h"
#include "GevcuTask.h"
#include "gevcu_idx_v_struct.h"
#include "morse.h"
#include "adcparamsinit.h"

static void new_state(struct GEVCUFUNCTION* pcf, uint32_t newstate);


/* *************************************************************************
 * @brief	: 
 * *************************************************************************/

void GevcuStates_otosettling_init(struct GEVCUFUNCTION* pcf)
{
	return;
}

/* ==== DISCONNECTED ======================================== */
void GevcuStates_disconnected(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* *************************************************************************
 * void GevcuStates_connecting(struct GEVCUFUNCTION* pcf);
 * @brief	: CONNECTING state
 * *************************************************************************/
/* ===== xCONNECTING ==================================================== */
static void transition_connecting(struct GEVCUFUNCTION* pcf)
{ // Intialize disconnected state
	return;
}
/* ====== CONNECTING ==================================================== */
void GevcuStates_connecting(struct GEVCUFUNCTION* pcf)
{
	return;
}		
/* ======= CONNECTED ==================================================== */
void GevcuStates_connected(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* ===== xDISCONNECTING ================================================= */
static void transition_disconnecting(struct GEVCUFUNCTION* pcf)
{
		return;	
}
/* ===== DISCONNECTING ================================================== */
void GevcuStates_disconnecting(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* ===== xFAULTING ====================================================== */
void transition_faulting(struct GEVCUFUNCTION* pcf, uint8_t fc)
{
		return;	
}
/* ===== FAULTING ======================================================= */
void GevcuStates_faulting(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* ===== FAULTED ======================================================= */
void GevcuStates_faulted(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* ===== RESET ========================================================= */
void GevcuStates_reset(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* *************************************************************************
 * static void open_contactors(struct GEVCUFUNCTION* pcf);
 * @brief	: De-energize contactors and set time delay for opening
 * @param	: pcf = pointer to struct with "everything" for this function
 * *************************************************************************/
static void open_contactors(struct GEVCUFUNCTION* pcf)
{
	return;
}
/* *************************************************************************
 * static void new_state(struct GEVCUFUNCTION* pcf, uint32_t newstate);
 * @brief	: When there is a state change, do common things
 * @param	: pcf = pointer to struct with "everything" for this function
 * @param	: newstate = code number for the new state
 * *************************************************************************/
static void new_state(struct GEVCUFUNCTION* pcf, uint32_t newstate)
{
	return;
}

