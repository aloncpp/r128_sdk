/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 */

#ifndef __EXPAND_CMD_H__
#define __EXPAND_CMD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <wifimg.h>

wmg_status_t wmg_send_expand_cmd(char *expand_cmd, void *expand_cb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __EXPAND_CMD_H__ */
