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

#ifndef _RINGBUFF_H_
#define _RINGBUFF_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RB_OK      (0)
#define RB_FAIL    (-1)
#define RB_DONE    (-2)
#define RB_ABORT   (-3)
#define RB_TIMEOUT (-4)

struct ringbuf {
    char *p_o;                  /**< Original pointer */
    char *volatile p_r;         /**< Read pointer */
    char *volatile p_w;         /**< Write pointer */
    volatile uint32_t fill_cnt; /**< Number of filled slots */
    uint32_t size;              /**< Buffer size */
    XR_OS_Semaphore_t can_read;
    XR_OS_Semaphore_t can_write;
    XR_OS_Mutex_t lock;
    bool abort_read;
    bool abort_write;
    bool is_done_write; /**< To signal that we are done writing */
};

typedef struct ringbuf* ringbuf_handle_t;

/**
 * @brief      Create ringbuffer with total size
 *
 * @param[in]  total_size   Size of ringbuffer
 *
 * @return     ringbuf_handle_t
 */
ringbuf_handle_t rb_create(int total_size);

/**
 * @brief      Cleanup and free all memory created by ringbuf_handle_t
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - RB_OK
 *     - RB_FAIL
 */
int rb_destroy(ringbuf_handle_t rb);

/**
 * @brief      Reset ringbuffer, clear all values as initial state
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return
 *     - RB_OK
 *     - RB_FAIL
 */
int rb_reset(ringbuf_handle_t rb);

/**
 * @brief      Get total bytes available of Ringbuffer
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return     total bytes available
 */
int rb_get_remain_bytes(ringbuf_handle_t rb);

/**
 * @brief      Get the number of bytes that have filled the ringbuffer
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return     The number of bytes that have filled the ringbuffer
 */
int rb_get_filled_bytes(ringbuf_handle_t rb);

/**
 * @brief      Get total size of Ringbuffer (in bytes)
 *
 * @param[in]  rb    The Ringbuffer handle
 *
 * @return     total size of Ringbuffer
 */
int rb_get_total_size(ringbuf_handle_t rb);

/**
 * @brief      Read from Ringbuffer to `buf` with len and wait `tick_to_wait` ticks until enough bytes to read
 *             if the ringbuffer bytes available is less than `len`.
 *             If `buf` argument provided is `NULL`, then ringbuffer do pseudo reads by simply advancing pointers.
 *
 * @param[in]  rb             The Ringbuffer handle
 * @param      buf            The buffer pointer to read out data
 * @param[in]  len            The length request
 * @param[in]  timeout        The time to wait
 *
 * @return     Number of bytes read
 */
int rb_read(ringbuf_handle_t rb, uint8_t *buf, int buf_len, uint32_t timeout);

/**
 * @brief      Write to Ringbuffer from `buf` with `len` and wait `tick_to_wait` ticks until enough space to write
 *             if the ringbuffer space available is less than `len`
 *
 * @param[in]  rb             The Ringbuffer handle
 * @param      buf            The buffer
 * @param[in]  len            The length
 * @param[in]  timeout        The time to wait
 *
 * @return     Number of bytes written
 */
int rb_write(ringbuf_handle_t rb, uint8_t *buf, int buf_len, uint32_t timeout);

/**
 * @brief      unblock Read from Ringbuffer to `buf` with len
 *
 * @param[in]  rb             The Ringbuffer handle
 * @param      buf            The buffer pointer to read out data
 * @param[in]  len            The length request
 *
 * @return     Number of bytes read
 */
int rb_unblock_read(ringbuf_handle_t rb, uint8_t *buf, int buf_len);

/**
 * @brief      unblock Write to Ringbuffer from `buf` with len
 *
 * @param[in]  rb             The Ringbuffer handle
 * @param      buf            The buffer
 * @param[in]  len            The length
 *
 * @return     Number of bytes written
 */
int rb_unblock_write(ringbuf_handle_t rb, uint8_t *buf, int buf_len);

#ifdef __cplusplus
}
#endif

#endif
