/** @file  shell_bt.h
 *  @brief Bluetooth shell functions
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SHELL_BT_H
#define __SHELL_BT_H

extern const struct shell *ctx_shell;
extern struct bt_conn *default_conn;

void conn_addr_str(struct bt_conn *conn, char *addr, size_t len);

#endif /* __SHELL_BT_H */
