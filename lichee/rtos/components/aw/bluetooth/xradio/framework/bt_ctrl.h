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

#ifndef _BT_CTRL_H_
#define _BT_CTRL_H_


#ifdef __cplusplus
extern "C" {
#endif

//#include "driver/chip/chip.h"

#ifdef CONFIG_BLE_FEATURE

#define BT_PUBLIC_ADDR_NONE			(0x0U)
#define BT_PUBLIC_ADDR_BY_WLAN		(0x1U)
#define BT_PUBLIC_ADDR_BY_EFUSE		(0x2U)

/**
 * @brief Modify the address from wlan address.
 * @note Default bt public address is wlan mac address +1 if
 *       BT_PUBLIC_ADDR_BY_WLAN is selected. Redefine this function
 *       can redefine the rule of difference between bt & wlan address.
 * @param addr: Wlan mac address
 */
void bt_public_addr_modify(uint8_t *addr);

/**
 * @brief Enable Bluetooth Controller
 * @retval int: 0 is success
 */
int bt_ctrl_enable();

/**
 * @brief Disable Bluetooth Controller
 * @retval int: 0 is success
 */
int bt_ctrl_disable(void);

/**
 * @brief Set Bluetooth Controller Mac
 */
void bt_ctrl_set_mac(uint8_t *mac);

int bt_zephyr_adapter_register(void);
int bt_zephyr_adapter_unregister(void);
int bt_bluedroid_adapter_register(void);
int bt_bluedroid_adapter_unregister(void);

#endif /* CONFIG_BLE_FEATURE */

#ifdef __cplusplus
}
#endif

#endif /* _BT_CTRL_H_ */
