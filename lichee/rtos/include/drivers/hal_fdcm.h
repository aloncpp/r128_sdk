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
#ifndef _HAL_FDCM_H_
#define _HAL_FDCM_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FDCM_DBG_ON     0
#define FDCM_WRN_ON     0
#define FDCM_ERR_ON     1
#define FDCM_ABORT_ON   0

#define FDCM_SYSLOG     printf
#define FDCM_ABORT()    while (1)

#define FDCM_LOG(flags, fmt, arg...)    \
    do {                                \
        if (flags)                      \
            FDCM_SYSLOG(fmt, ##arg);    \
    } while (0)

#define FDCM_DBG(fmt, arg...)   FDCM_LOG(FDCM_DBG_ON, "[FDCM] "fmt, ##arg)
#define FDCM_WRN(fmt, arg...)   FDCM_LOG(FDCM_WRN_ON, "[FDCM W] "fmt, ##arg)
#define FDCM_ERR(fmt, arg...)                           \
    do {                                                \
        FDCM_LOG(FDCM_ERR_ON, "[FDCM E] %s():%d, "fmt,  \
                 __func__, __LINE__, ##arg);            \
        if (FDCM_ABORT_ON)                              \
            FDCM_ABORT();                               \
    } while (0)

/**
 * @brief FDCM handle definition
 */
typedef struct fdcm_handle {
	uint32_t    flash;
	uint32_t    addr;
	uint32_t    size;
} fdcm_handle_t;

/* addr and size must be multiples of 64 */
extern fdcm_handle_t *fdcm_open(uint32_t flash, uint32_t addr, uint32_t size);
extern uint32_t fdcm_read(fdcm_handle_t *hdl, void *data, uint16_t data_size);
extern uint32_t fdcm_write(fdcm_handle_t *hdl, const void *data, uint16_t data_size);
extern int fdcm_erase(fdcm_handle_t *hdl);
extern void fdcm_close(fdcm_handle_t *hdl);
#ifdef __cplusplus
}
#endif

#endif /* _HAL_FDCM_H_ */