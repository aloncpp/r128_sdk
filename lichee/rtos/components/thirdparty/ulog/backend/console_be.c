/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-09-04     armink       the first version
 */

#include <ulog.h>
#include <syslog.h>
#include <console.h>
#include <stdio.h>

#ifdef CONFIG_ULOG_BACKEND_USING_CONSOLE

#if defined(ULOG_ASYNC_OUTPUT_BY_THREAD) && ULOG_ASYNC_OUTPUT_THREAD_STACK < 384
#error "The thread stack size must more than 384 when using async output by thread (ULOG_ASYNC_OUTPUT_BY_THREAD)"
#endif

static struct ulog_backend console;

void ulog_console_backend_output(struct ulog_backend *backend, rt_uint32_t level, const char *tag, rt_bool_t is_raw,
        const char *log, size_t len)
{
    /* int uart_puts(const char *string); */
    /* uart_puts(log);*/
    printf(log);
}

int ulog_console_backend_init(void)
{
    console.output = ulog_console_backend_output;

    ulog_backend_register(&console, "console", RT_TRUE);

    return 0;
}

#endif /* CONFIG_ULOG_BACKEND_USING_CONSOLE */
