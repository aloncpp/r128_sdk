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

#ifndef _BTMG_GATT_DB_H_
#define _BTMG_GATT_DB_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "ble/bluetooth/gatt.h"

#define GET_PRIV(base)             ((ll_stack_gatt_server_t *)__containerof(base, ll_stack_gatt_server_t, base))
#define GATT_ATTR_GET(base, index) base->get(base, index)
#define GATT_ATTR_DESTROY(base)    base->destroy(base)

typedef enum {
    GATT_ATTR_SERVICE,
    GATT_ATTR_CHARACTERISTIC,
    GATT_ATTR_CCC,
} gatt_attr_type;

typedef struct {
    gatt_attr_type type;
    uint8_t properties;
    struct bt_gatt_attr attr;
} ll_bt_gatt_attr_t;

typedef struct gatt_attr_base {
    int (*add)(struct gatt_attr_base *base, ll_bt_gatt_attr_t *param);
    struct bt_gatt_attr *(*get)(struct gatt_attr_base *base, uint32_t attr_index);
    void (*destroy)(struct gatt_attr_base *base);
} gatt_attr_base;

typedef union {
    struct bt_uuid_128 uuid;  //server
    struct bt_gatt_chrc chrc; //chrc
    struct _bt_gatt_ccc ccc;  //ccc
} gatt_attr_user_data_t;

typedef struct {
    uint32_t attrIndex;
    uint8_t *entry;
    struct bt_gatt_attr *attrs;
    struct bt_uuid_128 *uuids;
    gatt_attr_user_data_t *user_data;
    gatt_attr_base base;
    gatt_attr_base *attrBase;
    struct bt_gatt_service service;
    size_t max_attr_count;
} ll_stack_gatt_server_t;

int ll_gatt_attr_create(ll_stack_gatt_server_t *gatt_attr, uint32_t attr_num);
struct bt_gatt_attr *gatt_server_handle_to_attr(ll_stack_gatt_server_t *gatt_attr, uint16_t handle);

#ifdef __cplusplus
}
#endif

#endif /* _BTMG_GATT_DB_H_ */
