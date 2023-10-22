#ifndef _SUNXI_AMP_H
#define _SUNXI_AMP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <semphr.h>

#include <aw_list.h>
#include <hal_msgbox.h>

#include "sunxi_amp_status.h"
#include "sunxi_amp_msg.h"
#include "sunxi_amp_table.h"

#include <sunxi_hal_common.h>

#include <hal_thread.h>

enum {
    AMP_MISC_CMD_RV_CALL_M33_STRESS_TEST = 0,
    AMP_MISC_CMD_DSP_CALL_M33_STRESS_TEST,
    AMP_MISC_CMD_M33_CALL_RV_STRESS_TEST,
    AMP_MISC_CMD_DSP_CALL_RV_STRESS_TEST,
    AMP_MISC_CMD_M33_CALL_DSP_STRESS_TEST,
    AMP_MISC_CMD_RV_CALL_DSP_STRESS_TEST,
    AMP_MISC_CMD_RV_CALL_M33_CALL_RV_STRESS_TEST,
    AMP_MISC_CMD_RV_CALL_DSP_CALL_RV_STRESS_TEST,
    AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_STRESS_TEST,
    AMP_MISC_CMD_RV_CALL_M33_CALL_DSP_CALL_RV_STRESS_TEST,
    AMP_MISC_CMD_NUM
};

int misc_ioctrl(int cmd, void *data, int data_len);

typedef int (*sunxi_amp_dev_init)(void);

typedef unsigned long (*sunxi_amp_func)(void);
typedef unsigned long (*sunxi_amp_func0)(void);
typedef unsigned long (*sunxi_amp_func1)(void *args0);
typedef unsigned long (*sunxi_amp_func2)(void *args0, void *args1);
typedef unsigned long (*sunxi_amp_func3)(void *args0, void *args1, void *args2);
typedef unsigned long (*sunxi_amp_func4)(void *args0, void *args1, void *args2, void *args3);
typedef unsigned long (*sunxi_amp_func5)(void *args0, void *args1, void *args2, void *args3,
        void *args4);
typedef unsigned long (*sunxi_amp_func6)(void *args0, void *args1, void *args2, void *args3,
        void *args4, void *args5);
typedef unsigned long (*sunxi_amp_func7)(void *args0, void *args1, void *args2, void *args3,
        void *args4, void *args5, void *args6);
typedef unsigned long (*sunxi_amp_func8)(void *args0, void *args1, void *args2, void *args3,
        void *args4, void *args5, void *args6, void *args7);

typedef struct _sunxi_amp_service
{
    struct list_head i_list;
    uint32_t id: 8;
    sunxi_amp_service_func handle_invocation;
} sunxi_amp_service;

typedef struct _sunxi_amp_wait_t
{
    struct list_head i_list;
    TaskHandle_t task;
    uint32_t flags;
    sunxi_amp_msg msg;
    SemaphoreHandle_t signal;
} sunxi_amp_wait;

typedef struct _sunxi_amp_info_t
{
    QueueHandle_t send_queue; /*send to remote processor */
    QueueHandle_t recv_queue; /*receive from remote processor */
    TaskHandle_t  sendTask;   /*send to remote processor */
    TaskHandle_t  recvTask;   /*receive from remote processor */
    struct msg_endpoint sedp_arm;
    struct msg_endpoint sedp_rv;
    struct msg_endpoint sedp_dsp;
    sunxi_amp_wait wait;
    QueueHandle_t amp_msg_heap_mutex;
} sunxi_amp_info;

typedef int (*sunxi_amp_msg_func)(sunxi_amp_info *amp, sunxi_amp_msg *msg);

typedef struct _sunxi_amp_msg_ops
{
    sunxi_amp_msg_func send_to_queue;
    sunxi_amp_msg_func send_to_dev;
    sunxi_amp_msg_func receive_from_dev;
    sunxi_amp_dev_init init;
} sunxi_amp_msg_ops;

typedef struct
{
    sunxi_amp_func_table *RPCHandler_FSYS;
    sunxi_amp_func_table *RPCHandler_NET;
    sunxi_amp_func_table *RPCHandler_BT;
    sunxi_amp_func_table *RPCHandler_DEMO;
    sunxi_amp_func_table *RPCHandler_ARM_CONSOLE;
    sunxi_amp_func_table *RPCHandler_DSP_CONSOLE;
    sunxi_amp_func_table *RPCHandler_RV_CONSOLE;
    sunxi_amp_func_table *RPCHandler_PMOFM33;
    sunxi_amp_func_table *RPCHandler_PMOFRV;
    sunxi_amp_func_table *RPCHandler_PMOFDSP;
    sunxi_amp_func_table *RPCHandler_FLASHC;
    sunxi_amp_func_table *RPCHandler_M33_MISC;
    sunxi_amp_func_table *RPCHandler_RV_MISC;
    sunxi_amp_func_table *RPCHandler_DSP_MISC;
    sunxi_amp_func_table *RPCHandler_AUDIO;
    sunxi_amp_func_table *RPCHandler_RPDATA;
    sunxi_amp_func_table *RPCHandler_TFM;
} RPCHandler_RPCT_t;

#define FLAG_IS_BUF (1 << 0) /* it is a buffer point */
#define FLAG_SER_RD (1 << 1) /* the buffer need to be read in ser */
#define FLAG_SER_WR (1 << 2) /* the buffer need to be write in ser */
#define FLAG_NONE   (0) /* it is a value or no NULL */

#define ISBUF(flag)               (flag & FLAG_IS_BUF)
#define STUB_CACHE_CLEAN(flag)    (flag & FLAG_SER_RD)
#define STUB_CACHE_INVALID(flag)  (flag & FLAG_SER_WR)
#define SER_CACHE_INVALID(flag)   (flag & (FLAG_SER_RD | FLAG_SER_WR))
#define SER_CACHE_CLEAN(flag)     (flag & FLAG_SER_WR)

#ifdef CONFIG_ARCH_RISCV_C906
#define SELF_DIRECTION RPC_MSG_DIR_RV
#define SELF_NAME "RV"
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define SELF_DIRECTION RPC_MSG_DIR_CM33
#define SELF_NAME "M33"
#elif defined(CONFIG_ARCH_DSP)
#define SELF_DIRECTION RPC_MSG_DIR_DSP
#define SELF_NAME "DSP"
#else
#error "can not set SELF_DIRECTION"
#endif

#define AMP_QUEUE_LENGTH (256)

#define amp_err(fmt, args...) \
    printf("[%s:AMP_ERR][%s:%d]" fmt, SELF_NAME, __FUNCTION__, __LINE__, ##args)

#ifdef AMP_DEBUG
#define amp_debug(fmt, args...) \
    printf("[%s:AMP_DBG][%s:%d]" fmt, SELF_NAME, __FUNCTION__, __LINE__, ##args)
#define dump_amp_msg(x) _dump_amp_msg(x)
#define dump_amp_wait(x) _dump_amp_wait(x)
#else
#define amp_debug(fmt, args...)
#define dump_amp_msg(x)
#define dump_amp_wait(x)
#endif

#ifdef AMP_INFO
#define amp_info(fmt, args...) \
    printf("[%s:AMP_INFO][%s:%d]" fmt, SELF_NAME, __FUNCTION__, __LINE__, ##args)
#else
#define amp_info(fmt, args...)
#endif

#define RPCNO(base,type,isr)    (base+(((long)(&(((type*)0)->isr))) / sizeof(sunxi_amp_func_table)))

#define RPC_FIRST_NO(base,type,isr)    (base+(((long)(&(((type*)0)->isr))) / sizeof(sunxi_amp_func_table *)))

#define RPCCALL_RPCNO(x)        (RPC_FIRST_NO(0, RPCHandler_RPCT_t, x) << 8)

#define RPCNO_FSYS          RPCCALL_RPCNO(RPCHandler_FSYS)
#define RPCNO_NET           RPCCALL_RPCNO(RPCHandler_NET)
#define RPCNO_BT            RPCCALL_RPCNO(RPCHandler_BT)
#define RPCNO_DEMO          RPCCALL_RPCNO(RPCHandler_DEMO)
#define RPCNO_ARM_CONSOLE   RPCCALL_RPCNO(RPCHandler_ARM_CONSOLE)
#define RPCNO_DSP_CONSOLE   RPCCALL_RPCNO(RPCHandler_DSP_CONSOLE)
#define RPCNO_RV_CONSOLE    RPCCALL_RPCNO(RPCHandler_RV_CONSOLE)
#define RPCNO_PMOFRV        RPCCALL_RPCNO(RPCHandler_PMOFRV)
#define RPCNO_PMOFM33       RPCCALL_RPCNO(RPCHandler_PMOFM33)
#define RPCNO_PMOFDSP       RPCCALL_RPCNO(RPCHandler_PMOFDSP)
#define RPCNO_FLASHC        RPCCALL_RPCNO(RPCHandler_FLASHC)
#define RPCNO_M33_MISC      RPCCALL_RPCNO(RPCHandler_M33_MISC)
#define RPCNO_RV_MISC       RPCCALL_RPCNO(RPCHandler_RV_MISC)
#define RPCNO_DSP_MISC      RPCCALL_RPCNO(RPCHandler_DSP_MISC)
#define RPCNO_AUDIO         RPCCALL_RPCNO(RPCHandler_AUDIO)
#define RPCNO_RPDATA        RPCCALL_RPCNO(RPCHandler_RPDATA)
#define RPCNO_TFM           RPCCALL_RPCNO(RPCHandler_TFM)

#define RPCSERVICE_FSYS_DIR (RPC_MSG_DIR_RV)

#define RPCSERVICE_TFM_DIR (RPC_MSG_DIR_CM33)

#ifdef CONFIG_ARCH_RISCV_C906
#define RPCSERVICE_NET_DIR  (RPC_MSG_DIR_CM33)
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define RPCSERVICE_NET_DIR  (RPC_MSG_DIR_RV)
#endif
#ifdef CONFIG_ARCH_RISCV_C906
#define RPCSERVICE_BT_DIR  (RPC_MSG_DIR_CM33)
#elif defined(CONFIG_ARCH_ARM_CORTEX_M33)
#define RPCSERVICE_BT_DIR  (RPC_MSG_DIR_RV)
#endif

#define RPCSERVICE_PMOFRV_DIR  (RPC_MSG_DIR_RV)
#define RPCSERVICE_PMOFM33_DIR (RPC_MSG_DIR_CM33)
#define RPCSERVICE_PMOFDSP_DIR (RPC_MSG_DIR_DSP)

#define RPCSERVICE_DEMO_DIR (RPC_MSG_DIR_RV)
#define RPCSERVICE_ARM_CONSOLE_DIR (RPC_MSG_DIR_CM33)
#define RPCSERVICE_RV_CONSOLE_DIR  (RPC_MSG_DIR_RV)
#define RPCSERVICE_DSP_CONSOLE_DIR (RPC_MSG_DIR_DSP)

#define RPCSERVICE_FLASHC_DIR      (RPC_MSG_DIR_CM33)
#define RPCSERVICE_RV_MISC_DIR     (RPC_MSG_DIR_RV)
#define RPCSERVICE_M33_MISC_DIR    (RPC_MSG_DIR_CM33)
#define RPCSERVICE_DSP_MISC_DIR    (RPC_MSG_DIR_DSP)

#ifdef CONFIG_AMP_AUDIO_STUB

#ifdef CONFIG_AMP_AUDIO_REMOTE_DIR_CM33
#define RPCSERVICE_AUDIO_DIR       (RPC_MSG_DIR_CM33)
#elif defined(CONFIG_AMP_AUDIO_REMOTE_DIR_RV)
#define RPCSERVICE_AUDIO_DIR       (RPC_MSG_DIR_RV)
#elif defined(CONFIG_AMP_AUDIO_REMOTE_DIR_DSP)
#define RPCSERVICE_AUDIO_DIR       (RPC_MSG_DIR_DSP)
#else
#error "unknown remote dir"
#endif

#endif

#define RPCCALL_FSYS(y)     (RPCNO(RPCNO_FSYS, RPCHandler_FSYS_t, y) | (RPCSERVICE_FSYS_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_NET(y)      (RPCNO(RPCNO_NET,  RPCHandler_NET_t,  y) | (RPCSERVICE_NET_DIR << 29)  | SELF_DIRECTION << 26)
#define RPCCALL_BT(y)       (RPCNO(RPCNO_BT,   RPCHandler_BT_t,  y)  | (RPCSERVICE_BT_DIR << 29)   | SELF_DIRECTION << 26)
#define RPCCALL_DEMO(y)     (RPCNO(RPCNO_DEMO, RPCHandler_DEMO_t, y) | (RPCSERVICE_DEMO_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_ARM_CONSOLE(y)  (RPCNO(RPCNO_ARM_CONSOLE, RPCHandler_ARM_CONSOLE_t, y) | (RPCSERVICE_ARM_CONSOLE_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_DSP_CONSOLE(y)  (RPCNO(RPCNO_DSP_CONSOLE, RPCHandler_DSP_CONSOLE_t, y) | (RPCSERVICE_DSP_CONSOLE_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_RV_CONSOLE(y)   (RPCNO(RPCNO_RV_CONSOLE, RPCHandler_RV_CONSOLE_t, y) | (RPCSERVICE_RV_CONSOLE_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_PMOFRV(y)   (RPCNO(RPCNO_PMOFRV,   RPCHandler_PMOFRV_t,  y)  | (RPCSERVICE_PMOFRV_DIR << 29)  | SELF_DIRECTION << 26)
#define RPCCALL_PMOFM33(y)  (RPCNO(RPCNO_PMOFM33,  RPCHandler_PMOFM33_t,  y) | (RPCSERVICE_PMOFM33_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_PMOFDSP(y)  (RPCNO(RPCNO_PMOFDSP,  RPCHandler_PMOFDSP_t,  y) | (RPCSERVICE_PMOFDSP_DIR << 29) | SELF_DIRECTION << 26)
#define RPCCALL_FLASHC(y)   (RPCNO(RPCNO_FLASHC,   RPCHandler_FLASHC_t,  y)  | (RPCSERVICE_FLASHC_DIR << 29)  | SELF_DIRECTION << 26)
#define RPCCALL_RV_MISC(y)   (RPCNO(RPCNO_RV_MISC,   RPCHandler_RV_MISC_t,  y)  | (RPCSERVICE_RV_MISC_DIR << 29)  | SELF_DIRECTION << 26)
#define RPCCALL_M33_MISC(y)   (RPCNO(RPCNO_M33_MISC,   RPCHandler_M33_MISC_t,  y)  | (RPCSERVICE_M33_MISC_DIR << 29)  | SELF_DIRECTION << 26)
#define RPCCALL_DSP_MISC(y)   (RPCNO(RPCNO_DSP_MISC,   RPCHandler_DSP_MISC_t,  y)  | (RPCSERVICE_DSP_MISC_DIR << 29)  | SELF_DIRECTION << 26)
#define RPCCALL_TFM(y)       (RPCNO(RPCNO_TFM,   RPCHandler_TFM_t,  y)  | (RPCSERVICE_TFM_DIR << 29)   | SELF_DIRECTION << 26)
#ifdef CONFIG_AMP_AUDIO_STUB
#define RPCCALL_AUDIO(y)       	(RPCNO(RPCNO_AUDIO,   RPCHandler_AUDIO_t,  y)  | (RPCSERVICE_AUDIO_DIR << 29)   | SELF_DIRECTION << 26)
#endif
#define RPCCALL_RPDATA(y, dir)       (RPCNO(RPCNO_RPDATA,   RPCHandler_RPDATA_t,  y)  | (dir << 29)   | SELF_DIRECTION << 26)

#define SUNXI_AMP_ALIGNED __attribute__((aligned(8)))

#define SUNXI_AMP_SH_MAX_CMD_ARGS 16

#if 0
#define MAYBE_STATIC static
#define _MAYBE_STATIC
#else
#define MAYBE_STATIC
#endif

#define MSGBUFFER_STEP_SIZE (1024)
#define DEFAULT_MSG_BUFFER_SIZE (4096)

#define AMP_SEND_TASK_PRIO (HAL_THREAD_PRIORITY_HIGHEST)
#define AMP_RECV_TASK_PRIO (HAL_THREAD_PRIORITY_HIGHEST)
#define AMP_SEND_TASK_STACK (1024)
#define AMP_RECV_TASK_STACK (1024)

#define AMP_RPC_HANDLE_TASK_STACK_SIZE (1024 * 4)

sunxi_amp_info *get_amp_info(void);
int hal_amp_msg_send(sunxi_amp_msg *msg);
sunxi_amp_msg_ops *get_msg_ops(void);
int hal_amp_msg_recv(sunxi_amp_msg *msg);

void _dump_amp_msg(sunxi_amp_msg *msg);
void *amp_align_malloc(int size);
void amp_align_free(void *ptr);
void *amp_malloc(int size);
void amp_free(void *ptr);

unsigned long func_stub(uint32_t id, int haveRet, int stub_args_num, void *stub_args[]);
unsigned long func_stub_test(uint32_t id, MsgBuf *msgBuffer, int haveRet, int stub_args_num, void *stub_args[]);

sunxi_amp_msg *allocate_amp_msg_memory(void);
void free_amp_msg_memory(sunxi_amp_msg *msg);

#endif
