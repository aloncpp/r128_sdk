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
#ifndef _HCI_LIST_H
#define _HCI_LIST_H
#include <stdbool.h>

#define HCI_PKT_TYPE 1
#define HCI_OPCODE_TYPE 2
#define HCI_HANDLE_TYPE 3

typedef struct hci_pkt {
    uint8_t *buff;
    uint32_t len;
} hci_pkt_t;

typedef union data {
    hci_pkt_t *pkt;
    uint32_t opcode;
    uint32_t handle;
} data_u;

struct hci_list_node_t {
    struct hci_list_node_t *next;
    data_u data;
};
typedef struct hci_list_node_t hci_list_node_t;
typedef struct hci_list_t {
    hci_list_node_t *head;
    hci_list_node_t *tail;
    uint8_t type;
    uint32_t length;
} hci_list_t;

void hci_free_node(hci_list_node_t *node, uint8_t type);

hci_list_t *hci_list_new(uint8_t type);

void hci_list_clear(hci_list_t *list);

void hci_list_free(hci_list_t *list);

bool hci_list_is_empty(const hci_list_t *list);

uint32_t hci_list_length(const hci_list_t *list);

bool hci_list_append(hci_list_t *list, void *data);

uint32_t hci_list_value_exist(hci_list_t *list, uint32_t data);

bool hci_list_remove_node(hci_list_t *list, uint32_t num);

#endif
