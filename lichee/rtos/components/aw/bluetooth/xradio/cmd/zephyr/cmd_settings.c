/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_SETTINGS
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "cmd_shell.h"

#include <zephyr.h>

#include <settings/settings.h>
#include "ble/sys/util.h"
#include "ble/sys/byteorder.h"
#if (CONFIG_BT_SETTINGS)
#include "ble/settings/settings.h"
#endif

static int cmd_settings_init(const struct shell *shell, size_t argc, char *argv[])
{
    //todo
    return 0;
}

#if (CONFIG_SETTINGS_RUNTIME)
static int cmd_settings_runtime_set(const struct shell *shell, size_t argc, char *argv[])
{
    int err;

    if (argc < 3) {
        shell_help(shell);
        return SHELL_CMD_HELP_PRINTED;
    }

    err = settings_runtime_set(argv[1], argv[2], strlen(argv[2]));
    if (err) {
        shell_error(shell, "Unable to set %s (err %d)", argv[1], err);
        return err;
    }

    return 0;
}

static int cmd_settings_runtime_get(const struct shell *shell, size_t argc, char *argv[])
{
    int err;
    uint8_t settings_data[50];

    if (argc < 2) {
        shell_help(shell);
        return SHELL_CMD_HELP_PRINTED;
    }

    err = settings_runtime_get(argv[1], &settings_data[0], sizeof(settings_data));
    if (err) {
        shell_error(shell, "Unable to set %s (err %d)", argv[1], err);
        return err;
    } else {
        shell_print(shell, "Get: %s", settings_data);
    }

    return 0;
}

static int cmd_settings_runtime_commit(const struct shell *shell, size_t argc, char *argv[])
{
    int err;

    if (argc < 2) {
        shell_help(shell);
        return SHELL_CMD_HELP_PRINTED;
    }

    err = settings_runtime_commit(argv[1]);
    if (err) {
        shell_error(shell, "Unable to commit %s (err %d)", argv[1], err);
        return err;
    } else {
        shell_print(shell, "Commit Success");
    }

    return 0;
}
#endif

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(settings_cmds,
    SHELL_CMD_ARG(init, NULL, HELP_NONE, cmd_settings_init, 1, 0),
#if (CONFIG_SETTINGS_RUNTIME)
    SHELL_CMD_ARG(settings_runtime_set, NULL, "<name> <data>", cmd_settings_runtime_set, 4, 0),
    SHELL_CMD_ARG(settings_runtime_get, NULL, "<name>", cmd_settings_runtime_get, 2, 0),
    SHELL_CMD_ARG(settings_runtime_commit, NULL, "<name>", cmd_settings_runtime_commit, 2, 0),
#endif
    SHELL_SUBCMD_SET_END
);

static int cmd_settings(const struct shell *shell, size_t argc, char **argv)
{
    if (argc == 1) {
        shell_help(shell);
        return SHELL_CMD_HELP_PRINTED;
    }

    shell_error(shell, "%s unknown parameter: %s", argv[0], argv[1]);

    return -EINVAL;
}

SHELL_CMD_REGISTER(settings, &settings_cmds, "Settings shell commands", cmd_settings);

enum cmd_status cmd_settings_exec(char *cmd)
{
    return cmd_exec_shell(cmd, shell_settings_cmds, cmd_nitems(shell_settings_cmds) - 1);
}
#endif
