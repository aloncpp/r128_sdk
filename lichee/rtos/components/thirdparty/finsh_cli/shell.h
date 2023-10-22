/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-06-02     Bernard      Add finsh_get_prompt function declaration
 */

#ifndef __SHELL_H__
#define __SHELL_H__

#include <FreeRTOS.h>
#include "finsh.h"

#if 0//#ifdef CONFIG_COMPONENTS_MULTI_CONSOLE
#include <cli_console.h>

#define rt_kprintf(fmt, ...) \
    do { \
       cli_console_printf(get_clitask_console(), fmt, ##__VA_ARGS__); \
    } while(0);
#else
#define rt_kprintf printf
#endif

#define rt_memmove memmove
#define rt_strncpy strncpy

#ifndef FINSH_THREAD_PRIORITY
#define FINSH_THREAD_PRIORITY 20
#endif
#ifndef FINSH_THREAD_STACK_SIZE
#define FINSH_THREAD_STACK_SIZE 2048
#endif
#ifndef FINSH_CMD_SIZE
#define FINSH_CMD_SIZE      80
#endif

#define FINSH_OPTION_ECHO   0x01

#define FINSH_PROMPT        finsh_get_prompt()
const char* finsh_get_prompt(void);
int finsh_set_prompt(const char * prompt);

#ifdef FINSH_USING_HISTORY
    #ifndef FINSH_HISTORY_LINES
        #define FINSH_HISTORY_LINES 5
    #endif
#endif

#ifdef FINSH_USING_AUTH
    #ifndef CONFIG_FINSH_PASSWORD_MAX
        #define FINSH_PASSWORD_MAX RT_NAME_MAX
    #else
        #define FINSH_PASSWORD_MAX CONFIG_FINSH_PASSWORD_MAX
    #endif
    #ifndef CONFIG_FINSH_PASSWORD_MIN
        #define FINSH_PASSWORD_MIN 6
    #else
        #define FINSH_PASSWORD_MIN CONFIG_FINSH_PASSWORD_MIN
    #endif
    #ifndef CONFIG_FINSH_DEFAULT_PASSWORD
        #define FINSH_DEFAULT_PASSWORD "rtthread"
    #else
        #define FINSH_DEFAULT_PASSWORD CONFIG_FINSH_DEFAULT_PASSWORD
    #endif
#endif /* FINSH_USING_AUTH */

#ifndef FINSH_THREAD_NAME
#define FINSH_THREAD_NAME   "tshell"
#endif

enum input_stat
{
    WAIT_NORMAL,
    WAIT_SPEC_KEY,
    WAIT_FUNC_KEY,
};
struct finsh_shell
{
    //struct rt_semaphore rx_sem;
    StaticSemaphore_t rx_sem;

    enum input_stat stat;

    uint8_t echo_mode;
    uint8_t prompt_mode;

#ifdef FINSH_USING_HISTORY
    uint16_t current_history;
    uint16_t history_count;

    char cmd_history[FINSH_HISTORY_LINES][FINSH_CMD_SIZE];
#endif

#ifndef FINSH_USING_MSH_ONLY
    struct finsh_parser parser;
#endif

    char line[FINSH_CMD_SIZE];
    uint16_t line_position;
    uint16_t line_curpos;

#if !defined(RT_USING_POSIX) && defined(RT_USING_DEVICE)
    rt_device_t device;
#endif

#ifdef FINSH_USING_AUTH
    char password[FINSH_PASSWORD_MAX];
#endif
};

void finsh_set_echo(uint32_t echo);
uint32_t finsh_get_echo(void);

int finsh_system_init(void);
void finsh_set_device(const char* device_name);
const char* finsh_get_device(void);

uint32_t finsh_get_prompt_mode(void);
void finsh_set_prompt_mode(uint32_t prompt_mode);

#ifdef FINSH_USING_AUTH
rt_err_t finsh_set_password(const char *password);
const char *finsh_get_password(void);
#endif

#endif

