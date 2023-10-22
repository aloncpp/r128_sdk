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

#include "cmd_util.h"
#include <console.h>
#include "cmd_sntp.h"
#include "sntp.h"
#include "kernel/os/os_thread.h"
#include "hal_time.h"

#ifndef configAPPLICATION_NORMAL_PRIORITY
#ifdef CONFIG_ARCH_SUN20IW2P1
#define configAPPLICATION_NORMAL_PRIORITY XR_OS_THREAD_PRIO_APP
#else
#define configAPPLICATION_NORMAL_PRIORITY (15)
#endif
#endif

#define SNTP_THREAD_STACK_SIZE        (1024)

#define SNTP_TEST_CNT 1000
#define SNTP_FAIL_NUM 50
static TaskHandle_t sntp_task = NULL;
static int g_sntp_exit;

#if CMD_DESCRIBE
#define sntp_help_info \
    "[start] start sntp demo\n" \
    "[stop]  stop sntp demo\n"
#endif /* CMD_DESCRIBE */

void sntp_run(void *arg)
{
    uint32_t fail_it = 0, success_it = 0;
    uint32_t test_cnt = SNTP_TEST_CNT;
    printf("<sntp> <request>\n");

    sntp_set_server(0, "203.107.6.88");
    sntp_set_server(1, "ntp5.aliyun.com");
    sntp_set_server(2, "tw.pool.ntp.org");

    while (1)
    {
        if (g_sntp_exit)
            break;
        test_cnt--;
        if (sntp_request(NULL) != 0) {
            fail_it++;
            if(fail_it >= SNTP_FAIL_NUM)
            {
                printf("<sntp> <response : fail>\n");
                break;
            }
            printf("<sntp> <try : %d>\n", fail_it);
        }
        else
        {
            sntp_time *time = (sntp_time *)sntp_obtain_time();
            success_it++;
            printf("<sntp> <response : success>\n");
            printf("===sntp->get time form network (year mon day week hour min sec===\n");
            printf("(%04d-%02d-%02d  %u   %02d:%02d:%02d)\n", time->year, time->mon, time->day, time->week, time->hour, time->min, time->sec);
        }
        if (test_cnt == 0) {
            printf("<sntp> teste end!!! success cnt:<%d>, fail cnt:<%d>\n", success_it, fail_it);
            break;
        }
        hal_msleep(500);
    }
exit:
    printf("<sntp>: exit\n");
    sntp_task = NULL;
    vTaskDelete(NULL);
}

int sntp_start(void)
{
    if(sntp_task != NULL)
    {
        printf("sntp task have been already created\n");
        return -1;
    }

    g_sntp_exit = 0;
    if (xTaskCreate(sntp_run, "sntp", SNTP_THREAD_STACK_SIZE, NULL, configAPPLICATION_NORMAL_PRIORITY, &sntp_task) != 1) {
        printf("sntp task create failed\n");
        return -1;
    }

    return 0;
}

int sntp_stop(void)
{
    if(sntp_task)
    {
        printf("stop sntp\n");
        g_sntp_exit = 1;
    }
    return 0;
}

static enum cmd_status cmd_sntp_help_exec(char *cmd)
{
#if CMD_DESCRIBE
	CMD_LOG(1, "%s\n", sntp_help_info);
#endif
	return CMD_STATUS_ACKED;
}

enum cmd_status cmd_sntp_exec(char *cmd)
{
    if (cmd_strcmp(cmd, "help") == 0) {
        cmd_sntp_help_exec(cmd);
        return CMD_STATUS_ACKED;
    } else if (cmd_strcmp(cmd, "start") == 0) {
        sntp_start();
    } else if (cmd_strcmp(cmd, "stop") == 0) {
        sntp_stop();
    } else {
        sntp_start();
    }
    return CMD_STATUS_OK;
}

static void sntp_exec(int argc, char *argv[])
{
    cmd2_main_exec(argc, argv, cmd_sntp_exec);
}

FINSH_FUNCTION_EXPORT_CMD(sntp_exec, sntp, sntp testcmd);
