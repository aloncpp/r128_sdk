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
#include <stdlib.h>
#include "hci_list.h"
#include <stdio.h>
#include <assert.h>

void hci_free_node(hci_list_node_t *node, uint8_t type)
{
    assert(node != NULL);

    if (type == HCI_PKT_TYPE && node->data.pkt != NULL) {
        free(node->data.pkt->buff);
        free(node->data.pkt);
    }
    free(node);
}

hci_list_t *hci_list_new(uint8_t type)
{
    hci_list_t *list = (hci_list_t *)malloc(sizeof(hci_list_t));
    if (!list) {
        return NULL;
    }

    list->length = 0;
    list->type = type;
    list->head = NULL;
    list->tail = NULL;

    return list;
}

void hci_list_clear(hci_list_t *list)
{
    assert(list != NULL);
    for (hci_list_node_t *node = list->head; node; node = list->head) {
        list->head = list->head->next;
        hci_free_node(node, list->type);
        list->length--;
    }
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->type = 0;
}

void hci_list_free(hci_list_t *list)
{
    if (!list) {
        return;
    }

    hci_list_clear(list);
    free(list);
}

bool hci_list_is_empty(const hci_list_t *list)
{
    assert(list != NULL);
    return (list->length == 0);
}

uint32_t hci_list_length(const hci_list_t *list)
{
    assert(list != NULL);
    return list->length;
}

bool hci_list_append(hci_list_t *list, void *data)
{
    assert(list != NULL);
    assert(data != NULL);

    hci_list_node_t *node = (hci_list_node_t*)malloc(sizeof(hci_list_node_t));
    if (!node) {
        return false;
    }
    node->next = NULL;
    if (list->type == HCI_PKT_TYPE) {
        node->data.pkt = (hci_pkt_t *)data;
    } else if (list->type == HCI_OPCODE_TYPE) {
        node->data.opcode = *(uint32_t *)data;
    } else if (list->type == HCI_HANDLE_TYPE) {
        node->data.handle = *(uint32_t *)data;
    } else {
        free(node);
        return false;
    }
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
    ++list->length;
    return true;
}

bool hci_list_remove_node(hci_list_t *list, uint32_t num)
{
    assert(list != NULL);

    if (hci_list_is_empty(list)) {
        printf("E <remove node> empty list\n");
        return false;
    }

    hci_list_node_t *pre = list->head;
    hci_list_node_t *node = list->head;

    for (int i = 0; i < num - 1; i++) {
        if (node == NULL) {
            printf("E <remove node> null node\n");
            return false;
        }
        pre = node;
        node = node->next;
    }

    if (list->tail == list->head) { //only one node
        list->tail = list->head = NULL;
    } else if (node == list->head) { //one more node,delete head node
        list->head = node->next;
    } else if (node == list->tail) { //one more node,delete tail node
        pre->next = NULL;
        list->tail = pre;
    } else {
        pre->next = node->next;
    }
    hci_free_node(node, list->type);
    list->length--;
    return true;
}

uint32_t hci_list_value_exist(hci_list_t *list, uint32_t value)
{
    assert(list != NULL);

    if (hci_list_is_empty(list)) {
        return 0;
    }

    uint32_t num = 0;

    for (hci_list_node_t *node = list->head; node; node = node->next) {
        num++;
        if (list->type == HCI_OPCODE_TYPE && node->data.opcode == value) {
            return num;
        } else if (list->type == HCI_HANDLE_TYPE && node->data.handle == value) {
            return num;
        }
    }

    return false;
}

