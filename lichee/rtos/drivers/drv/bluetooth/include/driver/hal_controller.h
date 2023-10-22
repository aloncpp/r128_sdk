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
#ifndef _HAL_CONTROLLER_
#define _HAL_CONTROLLER_

#include <stdint.h>
#include <stdbool.h>
//#include "xrbtc.h"

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief           This function is called to init bluetooth controller.
 *
 * @return
 *                  - 0 : Succeed
 *                  - 1 : init controller failed
 */
int hal_controller_init();

/**
 * @brief           This function is called to deinit bluetooth controller.
 *
 * @return
 *                  - 0 : Succeed
 *                  - 1 : deinit controller failed
 */
int hal_controller_deinit();

/**
 * @brief           This function is called to judge bluetooth controller is ready or not.
 *                  If controller is ready, return the type of drivers.
 *
 * @return
 *                  - 0 : controller is not ready
 *                  - 1 : controller is ready by vhci
 *                  - others : Reserved
 */
uint32_t hal_controller_ready(void);

/**
 * @brief           This function is called to set bluetooth controller mac address.
 *
 * @param[in]       mac - mac address
 */
void hal_controller_set_mac(uint8_t *mac);

#ifdef __cplusplus
 }
#endif

#endif /* _R128_FW_H_ */

