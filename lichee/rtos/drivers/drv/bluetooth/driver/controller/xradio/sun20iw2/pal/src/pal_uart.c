/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      UART driver definition.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "pal_uart.h"
#include "hal_uart.h"
#include "sunxi_hal_common.h"

/**************************************************************************************************
  Type definitions
**************************************************************************************************/

#define PAL_UART_DMA_EN           0
#define PAL_UART_IT_EN            1

#define TIMEOUT                   0x7FFFFFFF
/*! \brief      Control block. */
typedef struct {
	PalUartState_t  state;    /*!< UART state. */
	PalUartConfig_t config;   /*!< UART configuration. */
	uint8_t         inst;     /*!< UART driver instance. */
} palUartCtrlBlk_t;

/**************************************************************************************************
  Local Variables
**************************************************************************************************/

#ifdef DEBUG

/*! \brief      Parameter check. */
#define PAL_UART_PARAM_CHECK(p, expr)       { if (!(expr)) { p->state = PAL_UART_STATE_ERROR; return; } }

#else

/*! \brief      Parameter check (disabled). */
#define PAL_UART_PARAM_CHECK(p, expr)

#endif

static palUartCtrlBlk_t palUartCb[3];

/**************************************************************************************************
  Local Functions
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Get UART instance number from UART ID.
 *
 *  \param      id      UART ID.
 *
 *  \return     UART instance number.
 */
/*************************************************************************************************/
static palUartCtrlBlk_t *palUartGetContext(PalUartId_t id)
{
	switch (id) {
	case PAL_UART_ID_CHCI:
		return &palUartCb[2];
	case PAL_UART_ID_TERMINAL:
		return &palUartCb[0];
	case PAL_UART_ID_USER:
	default:
		break;
	}

	return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief      Initialize UART.
 *
 *  \param      id      UART ID.
 *  \param      pCfg    Peripheral configuration.
 */
/*************************************************************************************************/
void PalUartInit(PalUartId_t id, const PalUartConfig_t *pCfg)
{
	unsigned long status = SUNXI_HAL_ERROR;
	uart_port_t uartId;
	_uart_config_t uart_config;
	palUartCtrlBlk_t *pCtx = palUartGetContext(id);

	PAL_UART_PARAM_CHECK(pCtx, pCtx != NULL);
	PAL_UART_PARAM_CHECK(pCtx, pCfg != NULL);

	pCtx->config = *pCfg;

	/* Resolve instance. */
	switch (pCtx - palUartCb) {
	default:
	case 0:
		uartId = UART_0;
		break;
	case 1:
		uartId = UART_1;
		break;
	case 2:
		uartId = UART_2;
		break;
	}

	switch (pCfg->baud) {
	case 4800:
		uart_config.baudrate = UART_BAUDRATE_4800;
		break;
	case 9600:
		uart_config.baudrate = UART_BAUDRATE_9600;
		break;
	case 115200:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	case 1500000:
		uart_config.baudrate = UART_BAUDRATE_1500000;
		break;
	case 3000000:
		uart_config.baudrate = UART_BAUDRATE_3000000;
		break;
	default:
		uart_config.baudrate = UART_BAUDRATE_115200;
		break;
	}
	uart_config.word_length = UART_WORD_LENGTH_8;
	uart_config.stop_bit = UART_STOP_BIT_1;
	uart_config.parity = UART_PARITY_NONE;

	status = hal_uart_init(uartId);
	hal_uart_control(uartId, 0, &uart_config);
	if (pCfg->hwFlow) {
		hal_uart_set_hardware_flowcontrol(uartId);
	} else {
		hal_uart_disable_flowcontrol(uartId);
	}
	if (status == SUNXI_HAL_OK) {
		pCtx->inst = (uint8_t)uartId;
		pCtx->state = PAL_UART_STATE_READY;
	} else {
		pCtx->state = PAL_UART_STATE_ERROR;
	}

#if PAL_UART_DMA_EN
	status = XHAL_UART_EnableTxDMA(uartId);
	if (status != SUNXI_HAL_OK) {
		pCtx->state = PAL_UART_STATE_ERROR;
	  return ;
	}

	status = XHAL_UART_EnableRxDMA(uartId);
	if (status != SUNXI_HAL_OK) {
		pCtx->state = PAL_UART_STATE_ERROR;
	  return ;
	}
#endif

	return;
}

/*************************************************************************************************/
/*!
 *  \brief      De-Initialize UART.
 *
 *  \param      id      UART ID.
 */
/*************************************************************************************************/
void PalUartDeInit(PalUartId_t id)
{
	palUartCtrlBlk_t *pCtx = palUartGetContext(id);
	PAL_UART_PARAM_CHECK(pCtx, pCtx != NULL);
	PAL_UART_PARAM_CHECK(pCtx, pCtx->state == PAL_UART_STATE_READY);

	int status = hal_uart_deinit((int32_t)(pCtx->inst));
	PAL_UART_PARAM_CHECK(pCtx, status == SUNXI_HAL_OK);
	(void)status;

	pCtx->state = PAL_UART_STATE_UNINIT;
}

/*************************************************************************************************/
/*!
 *  \brief      Get the current state.
 *
 *  \param      id      UART id.
 *
 *  \return      Current state.
 */
/*************************************************************************************************/
PalUartState_t PalUartGetState(PalUartId_t id)
{
	palUartCtrlBlk_t *pCtx = palUartGetContext(id);

	if (pCtx == NULL) {
		return PAL_UART_STATE_ERROR;
	}

	return pCtx->state;
}

/*************************************************************************************************/
/*!
 *  \brief      Read data from Rx FIFO.
 *
 *  \param      id      UART ID.
 *  \param      pData   Read buffer.
 *  \param      len     Number of bytes to read.
 */
/*************************************************************************************************/
void PalUartReadData(PalUartId_t id, uint8_t *pData, uint16_t len)
{
	palUartCtrlBlk_t *pCtx = palUartGetContext(id);

	PAL_UART_PARAM_CHECK(pCtx, pCtx != NULL);
	PAL_UART_PARAM_CHECK(pCtx, pCtx->state != PAL_UART_STATE_UNINIT);
	PAL_UART_PARAM_CHECK(pCtx, pData != NULL);
	PAL_UART_PARAM_CHECK(pCtx, len > 0);

#if PAL_UART_DMA_EN
	XHAL_UART_Receive_DMA((XUART_ID)(pCtx->inst), pData, len, 0xFFFFFFFF);
#endif
#if PAL_UART_IT_EN
	hal_uart_receive_no_block((int32_t)(pCtx->inst), pData, (uint32_t)len, TIMEOUT);
#endif
	if (pCtx->config.rdCback != NULL) {
		pCtx->config.rdCback();
	}
}

/*************************************************************************************************/
/*!
 *  \brief      Write data to Tx FIFO.
 *
 *  \param      id      UART ID.
 *  \param      pData   Write buffer.
 *  \param      len     Number of bytes to write.
 */
/*************************************************************************************************/
void PalUartWriteData(PalUartId_t id, const uint8_t *pData, uint16_t len)
{
	palUartCtrlBlk_t *pCtx = palUartGetContext(id);

	PAL_UART_PARAM_CHECK(pCtx, pCtx != NULL);
	PAL_UART_PARAM_CHECK(pCtx, pCtx->state != PAL_UART_STATE_UNINIT);
	PAL_UART_PARAM_CHECK(pCtx, pData != NULL);
	PAL_UART_PARAM_CHECK(pCtx, len > 0);

#if PAL_UART_DMA_EN
	XHAL_UART_Transmit_DMA((XUART_ID)(pCtx->inst), pData, len);
#endif
#if PAL_UART_IT_EN
	hal_uart_send((int32_t)(pCtx->inst), pData, (uint32_t)len);
#endif
	if (pCtx->config.wrCback != NULL) {
		pCtx->config.wrCback();
	}
}

