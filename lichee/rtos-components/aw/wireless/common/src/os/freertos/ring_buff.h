/*
* Copyright (c) 2020 Allwinner Technology Co., Ltd. ALL rights reserved.
* Author: laumy liumingyuan@allwinnertech.com
* Date: 2020.04.25
* Description:ring buffer.
*/

#ifndef __RING_BUFF_H__
#define __RING_BUFF_H__
#ifdef __cplusplus
extern "C" {
#endif
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#define RING_BUFF_DEBUG 0

typedef enum {
    RB_SG_NONE = 0,
    RB_SG_NOMAL,
    RB_SG_1_4,
    RB_SG_1_2,
    RG_SG_2_3,
    RB_SG_QUIT,
} rb_signal_type_t;

typedef struct {
    bool enable;
    uint8_t *buff;
    uint32_t buff_size;
    uint8_t *write_pos;
    uint8_t *read_pos;
    uint32_t data_len;
    bool signal;
    pthread_mutex_t lock;
    pthread_cond_t cond;
    pthread_condattr_t cattr;

    bool p_wait;
    bool comsumer_ready;
    pthread_cond_t pcond;
    pthread_condattr_t pcattr;
} ring_buff_t;

int ring_buff_init(ring_buff_t *q, uint32_t data_size);

int ring_buff_deinit(ring_buff_t *q);

int ring_buff_signal(ring_buff_t *q, rb_signal_type_t signal);

int ring_buff_start(ring_buff_t *q);

int ring_buff_stop(ring_buff_t *q);

int ring_buff_put(ring_buff_t *q, void *data, uint32_t data_size, int timeout_ms,
                  rb_signal_type_t signal);

int ring_buff_get(ring_buff_t *q, void *data, uint32_t data_size, int timeout_ms, uint32_t cache);

uint32_t ring_buff_data_len(ring_buff_t *q);

uint32_t ring_buff_get_buff_size(ring_buff_t *q);
#ifdef __cplusplus
}
#endif
#endif
