/*************************************************************************************************/
/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*************************************************************************************************/

#ifndef _XRBTC_H
#define _XRBTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**************************************************************************************************
  Type Definitions
**************************************************************************************************/

/*! \brief      State define. */
typedef enum {
	XRBTC_STATE_SUCCESS      = 0,
	XRBTC_STATE_FAIL         = 1,
} xrbtcState_t;

/*! \brief      Hci callback define. */
typedef int (* xrbtc_hci_c2h)(uint8_t hciType, const uint8_t *pBuffStart, uint32_t buffOffset, uint32_t buffLen);
typedef int (* xrbtc_hci_h2c_cb)(uint8_t status, const uint8_t *pBuffStart, uint32_t buffOffset, uint32_t buffLen);

/**************************************************************************************************
  Function Declarations
**************************************************************************************************/

/*************************************************************************************************/
/*!
 *  \brief      Initialize the xradio bluetooth controller.
 *
 *  \return XRBTC_STATE_SUCCESS if initialize success, XRBTC_STATE_FAIL otherwise.
 *
 * One-time initialization of bluetooth controller resources.
 * This routine can be allocate memory, create task, load debug system.
 */
/*************************************************************************************************/
int xrbtc_init(void);

/*************************************************************************************************/
/*!
 *  \brief      Deinitialize the xradio bluetooth controller.
 *
 *  \return XRBTC_STATE_SUCCESS if deinitialize success, XRBTC_STATE_FAIL otherwise.
 *
 * One-time deinitialization of baseband resources.
 * This routine can be free memory, delete task, unload debug system.
 * Make sure bluetooth disbale before calling.
 */
/*************************************************************************************************/
int xrbtc_deinit(void);

/*************************************************************************************************/
/*!
 *  \brief      Enable the xradio bluetooth controller.
 *
 *  \return XRBTC_STATE_SUCCESS if enable success, XRBTC_STATE_FAIL otherwise.
 *
 * This routine can be used to enable baseband clock and power, initialize parameters.
 * Make sure bluetooth disbale before calling.
 */
/*************************************************************************************************/
int xrbtc_enable(void);

/*************************************************************************************************/
/*!
 *  \brief      Disable the xradio bluetooth controller.
 *
 *  \return XRBTC_STATE_SUCCESS if disable success, XRBTC_STATE_FAIL otherwise.
 *
 *  This routine can be used to disable baseband clock and power.
 */
/*************************************************************************************************/
int xrbtc_disable(void);

/*************************************************************************************************/
/*!
 *  \brief      Initialize HCI controller protocol and register callback function.
 *
 *  \param  hci_c2h       hci controller to host.
 *  \param  hci_h2c_cb    hci host to controller callback.
 *
 *  \return XRBTC_STATE_SUCCESS if success, XRBTC_STATE_FAIL otherwise.
 */
/*************************************************************************************************/
int xrbtc_hci_init(xrbtc_hci_c2h hci_c2h, xrbtc_hci_h2c_cb hci_h2c_cb);

/*************************************************************************************************/
/*!
 *  \brief  Deinitialize HCI controller protocol.
 *
 *  \param
 *
 *  \return XRBTC_STATE_SUCCESS if success, XRBTC_STATE_FAIL otherwise.
 */
/*************************************************************************************************/
int xrbtc_hci_deinit(void);

/*************************************************************************************************/
/*!
 *  \brief      Host send cmd or data to controller.
 *
 *  \param  hciType     hci type, 0x01: command, 0x02: acl data, 0x03: sync data.
 *  \param  pBuffStart  Transmit buffer start position.
 *  \param  buffOffset  Offset of valid data in transmit buffer.
 *  \param  buffLen     Length of valid data in transmit buffer.
 *
 *  \return XRBTC_STATE_SUCCESS if get success, XRBTC_STATE_FAIL otherwise.
 */
/*************************************************************************************************/
int xrbtc_hci_h2c(uint8_t hciType, const uint8_t *pBuffStart,
                  uint32_t buffOffset, uint32_t buffLen);

/*************************************************************************************************/
/*!
 *  \brief      Host receiver controller data callback.
 *
 *  \param  status      host receiver state, see xbtcState_t.
 *  \param  pBuffStart  Transmit buffer start position.
 *  \param  buffOffset  Offset of valid data in transmit buffer.
 *  \param  buffLen     Length of valid data in transmit buffer.
 *
 *  \return XRBTC_STATE_SUCCESS if success, XRBTC_STATE_FAIL otherwise.
 */
/*************************************************************************************************/
int xrbtc_hci_c2h_cb(uint8_t status, const uint8_t *pBuffStart,
                     uint32_t buffOffset, uint32_t buffLen);

/*************************************************************************************************/
/*!
 *  \brief      Initialize the enviroment for loading sdd data.
 *
 *  \param  size       	sdd data size.
 *
 *  \return XRBTC_STATE_SUCCESS if set success, XRBTC_STATE_FAIL otherwise.
 *
 *  should call this function before xrbtc_init.
 */
/*************************************************************************************************/
int32_t xrbtc_sdd_init(uint32_t size);

/*************************************************************************************************/
/*!
 *  \brief      write sdd data to bluetooth controller firmware.
 *
 *  \param  data       	sdd data.
 *  \param  len       	sdd data len.
 *
 *  \return XRBTC_STATE_SUCCESS if set success, XRBTC_STATE_FAIL otherwise.
 *
 *  should call this function before xrbtc_init.
 */
/*************************************************************************************************/
int32_t xrbtc_sdd_write(uint8_t *data, uint32_t len);

/*************************************************************************************************/
/*!
 *  \brief      Command interface.
 *
 *  \param  pCmd       command string.
 *
 *  \return XRBTC_STATE_SUCCESS if success, XRBTC_STATE_FAIL otherwise.
 */
/*************************************************************************************************/
int xrbtc_cmd_exec(uint8_t *pCmd);

#ifdef __cplusplus
};
#endif

#endif /* _XRBTC_H */

