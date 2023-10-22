/*
* Copyright (C) 2018 Realtek Semiconductor Corporation. All Rights Reserved.
*/

#include <string.h>

#include "bt_link.h"

T_BT_DATA bt_data;

T_BT_LINK *bt_find_link_by_addr(uint8_t *bd_addr)
{
    uint8_t i = 0;
    T_BT_LINK *p_link;

    for (i = 0; i < MAX_LINK_NUM; i++)
    {
        p_link = &bt_data.bt_link[i];

        if (memcmp(p_link->bd_addr, bd_addr, 6) == 0)
        {
            return p_link;
        }
    }

    return NULL;
}

T_BT_LINK *bt_alloc_link(uint8_t *bd_addr)
{
    uint8_t i;

    for (i = 0; i < MAX_LINK_NUM; i++)
    {
        if (bt_data.bt_link[i].acl_link_state == LINK_STATE_STANDBY)
        {
            bt_data.bt_link[i].acl_link_state = LINK_STATE_ALLOCATED;
            memcpy(bt_data.bt_link[i].bd_addr, bd_addr, 6);

            return &bt_data.bt_link[i];
        }
    }

    return NULL;
}

void bt_free_link(T_BT_LINK *p_link)
{
    memset((uint8_t *)p_link, 0, sizeof(T_BT_LINK));
}
