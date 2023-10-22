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

#ifndef _PLATFORM_BSP_H_
#define _PLATFORM_BSP_H_

#include <stdint.h>

struct xr_bluetooth_rf{
	void   (*init)(void);
	void   (*reset)(void);
	void   (*deinit)(void);
};

struct xr_bluetooth_uart {
	int    (*init)(void);
	void   (*disable_flowcontrol)(void);
	void   (*enable_flowcontrol)(void);
	int    (*send_data)(uint8_t *buf,  uint32_t count);
	int    (*receive_data)(uint8_t *buf,  uint32_t count);
	int    (*receive_data_no_block)(uint8_t *buf, uint32_t count, int32_t timeout);
	int    (*set_baudrate)(uint32_t baudrate);
	void   (*set_loopback)(bool enable);
	int    (*deinit)(void);
};

extern const struct xr_bluetooth_rf *xradio_get_platform_rf(void);
extern const struct xr_bluetooth_uart *xradio_get_platform_uart(void);

#endif /* _PLATFORM_BSP_H_ */
