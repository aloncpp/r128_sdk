#ifndef RT_CONFIG_H
#define RT_CONFIG_H

#include <console.h>
#include <cli_console.h>
#include <stdio.h>

#define RT_USING_POSIX
#define RT_USING_DEVICE
#define RT_USING_HEAP
//#define RT_USING_DFS
#define DFS_USING_WORKDIR

/* the buffer size of console */
#define RT_CONSOLEBUF_SIZE 1024

/* SECTION: finsh, a C-Express shell */
/* Using FinSH as Shell*/
#define RT_USING_FINSH
#define FINSH_USING_SYMTAB

#define FINSH_THREAD_NAME CONFIG_FINSH_THREAD_NAME

#ifdef CONFIG_FINSH_USING_HISTORY
#define FINSH_USING_HISTORY
#endif

#define FINSH_HISTORY_LINES CONFIG_FINSH_HISTORY_LINES

#ifdef CONFIG_FINSH_USING_DESCRIPTION
#define FINSH_USING_DESCRIPTION
#endif

/* FINSH_ECHO_DISABLE_DEFAULT is not set */
#define FINSH_THREAD_PRIORITY CONFIG_FINSH_THREAD_PRIORITY

#ifdef CONFIG_COMPONENTS_OPUS_TEST
#define FINSH_THREAD_STACK_SIZE 65535
#else
#define FINSH_THREAD_STACK_SIZE CONFIG_FINSH_THREAD_STACK_SIZE
#endif

#define FINSH_CMD_SIZE CONFIG_FINSH_CMD_SIZE

#ifdef CONFIG_FINSH_USING_AUTH
#define FINSH_USING_AUTH

#ifdef CONFIG_FINSH_ALLOW_SKIP_AUTH
#define FINSH_ALLOW_SKIP_AUTH
#endif

#endif

#define FINSH_USING_MSH
#define FINSH_USING_MSH_DEFAULT
#define FINSH_USING_MSH_ONLY

#define FINSH_ARG_MAX CONFIG_FINSH_ARG_MAX

#define RT_ALIGN_SIZE 4

#define ATTRIBUTE_ALIGN(n)          __attribute__((aligned(n)))
#define RT_GCC_ALIGN(n)             __attribute__((aligned(n)))

#define rt_calloc calloc
#define rt_malloc malloc
#define rt_free free
#define RT_ASSERT(x) configASSERT(x)
#define rt_strdup strdup
#define rt_strlen strlen
#define rt_strncmp strncmp
#define rt_memset memset
#define rt_hw_interrupt_enable vPortExitCritical
#define rt_hw_interrupt_disable vPortEnterCritical

typedef portBASE_TYPE rt_err_t;
typedef portBASE_TYPE rt_base_t;
typedef int8_t   rt_bool_t;
typedef size_t   rt_size_t;
#define RT_FALSE 0
#define RT_TRUE 1

#define RT_EOK                          0               /**< There is no error */
#define RT_ERROR                        1               /**< A generic error happens */

typedef TaskHandle_t rt_thread;
typedef BaseType_t rt_thread_t;

#define rt_inline static __inline

#endif
