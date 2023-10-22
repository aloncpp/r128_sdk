/** @file shell_gatt.h
 @brief GATT shell functions

 This is not to be included by the application.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SHELL_GATT_H
#define __SHELL_GATT_H

int cmd_gatt_show_db(int argc, char *argv[]);
int cmd_gatt_exchange_mtu(int argc, char *argv[]);
int cmd_gatt_discover(int argc, char *argv[]);
int cmd_gatt_read(int argc, char *argv[]);
int cmd_gatt_mread(int argc, char *argv[]);
int cmd_gatt_write(int argc, char *argv[]);
int cmd_gatt_write_without_rsp(int argc, char *argv[]);
int cmd_gatt_subscribe(int argc, char *argv[]);
int cmd_gatt_unsubscribe(int argc, char *argv[]);
int cmd_gatt_register_test_svc(int argc, char *argv[]);
int cmd_gatt_unregister_test_svc(int argc, char *argv[]);
int cmd_gatt_write_cmd_metrics(int argc, char *argv[]);

#endif /* __SHELL_GATT_H */
