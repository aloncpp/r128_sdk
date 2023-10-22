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

#ifndef _BT_LIB_H_
#define _BT_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "hal_hci.h"
#include "hal_controller.h"

#define BT_FEATURES_BLE    (0x01)
#define BT_FEATURES_BR     (0x10)
#define BT_FEATURES_DUAL   (0x11)

typedef struct bt_hc_callbacks {
	int (*data_ind)(const uint8_t *data, uint16_t len);
} bt_hc_callbacks_t;

typedef struct bt_hc_interface {
	int   (*open)(uint8_t features, bt_hc_callbacks_t *p_cb);
	int   (*write)(int id, uint8_t type, uint8_t *data, uint16_t len);
	int   (*read)(int id, uint8_t *data, uint16_t len);
	int   (*close)(int id);
} bt_hc_interface_t;

typedef struct bt_hc_ctrl_interface {
	uint32_t   (*status)(void);
	void       (*set_mac)(uint8_t *mac);
} bt_hc_ctrl_interface_t;

typedef struct bt_lib_interface {
	int   (*init)(void);
	int   (*deinit)(void);
	const bt_hc_interface_t *hci_ops;
	const bt_hc_ctrl_interface_t *hci_ctrl_ops;
} bt_lib_interface_t;

extern const bt_lib_interface_t *bt_lib_get_interface(void);

#ifdef __cplusplus
}
#endif

#endif

