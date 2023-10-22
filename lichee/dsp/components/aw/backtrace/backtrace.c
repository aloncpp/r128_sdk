#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <xtensa_context.h>
#include <backtrace.h>
#include <FreeRTOS.h>
#include <task.h>

#define AW_OK (0)
#define AW_FAIL (-1)

#define EXCCAUSE_INSTR_PROHIBITED   20
#define CONSOLE_UART (0)

#define PRINT_CALL(print_func, fmt, ...)                                       \
	if (print_func) {                                                      \
		print_func(fmt, ##__VA_ARGS__);                                \
	}

#define ALIGNUP(n, val) (((val) + (n)-1) & -(n))

typedef struct
{
    uint32_t pc;
    uint32_t sp;
    uint32_t next_pc;
} backtrace_frame_t;

typedef struct
{
    unsigned long start;
    unsigned long end;
} valid_address_t;

enum task_states
{
    TASK_SUSPEND = 0,
    TASK_READY,
    TASK_INTERRUPTED,
    TASK_RUNNING,
};

extern void esp_backtrace_get_start(uint32_t *pc, uint32_t *sp, uint32_t *next_pc);

#ifdef CONFIG_ARCH_SUN20IW2
static valid_address_t valid_addr [] = {
	{.start = 0x04000000, .end = 0x040FFFFF},
	{.start = 0x0C000000, .end = 0x0C7FFFFF},
};
#else
static valid_address_t valid_addr [] = {
	{.start = 0x00020000, .end = 0x00027FFF},
	{.start = 0x00028000, .end = 0x00037FFF},
	{.start = 0x00038000, .end = 0x0003FFFF},
	{.start = 0x00040000, .end = 0x00047FFF},
	{.start = 0x00400000, .end = 0x0040FFFF},
	{.start = 0x00420000, .end = 0x00427FFF},
	{.start = 0x00440000, .end = 0x00447FFF},
	{.start = 0x10000000, .end = 0x1FFFFFFF},
	{.start = 0x20020000, .end = 0x20027FFF},
	{.start = 0x20028000, .end = 0x20037FFF},
	{.start = 0x20038000, .end = 0x2003FFFF},
	{.start = 0x20040000, .end = 0x20047FFF},
	{.start = 0x30000000, .end = 0xFFFFFFFF},
};
#endif

static bool check_stack_ptr_is_sane(uint32_t ip)
{
    bool ret = 0;
    if ((ip & 0xF) != 0)
        return 0;

    int i;
    for(i = 0; i < sizeof(valid_addr)/sizeof(valid_addr[0]); i++) {
        if (ip >= valid_addr[i].start && ip <= valid_addr[i].end) {
            ret = 1;
            break;
        }
    }
    return ret;

}

uint32_t cpu_process_stack_pc(unsigned long pc)
{
    if (pc & 0x80000000)
    {
        pc = (pc & 0x3fffffff) | 0x00000000;
    }
    return pc;
}

bool check_ptr_is_valid(const void *p)
{
    bool ret = 0;

    intptr_t ip = (intptr_t) p;

    int i;
    for(i = 0; i < sizeof(valid_addr)/sizeof(valid_addr[0]); i++) {
        if (ip >= valid_addr[i].start && ip <= valid_addr[i].end) {
            ret = 1;
            break;
        }
    }
    return ret;

}

static bool backtrace_get_next_frame(backtrace_frame_t *frame)
{
    bool ret = 1;
    uint32_t remap_pc;
    void *base_save = (void *)frame->sp;
    frame->pc = frame->next_pc;
    frame->next_pc = *((unsigned long *)(base_save - 16));
    frame->sp =  *((uint32_t *)(base_save - 12));

    remap_pc = cpu_process_stack_pc(frame->pc);

    ret = check_ptr_is_valid((void *)remap_pc);
    if (ret == 0)
        return ret;

    ret = check_stack_ptr_is_sane(frame->sp);
    if (ret == 0)
        return ret;

    return ret;
}

static char *long2str(long num, char *str)
{
    char         index[] = "0123456789ABCDEF";
    unsigned long usnum   = (unsigned long)num;

    str[7] = index[usnum % 16];
    usnum /= 16;
    str[6] = index[usnum % 16];
    usnum /= 16;
    str[5] = index[usnum % 16];
    usnum /= 16;
    str[4] = index[usnum % 16];
    usnum /= 16;
    str[3] = index[usnum % 16];
    usnum /= 16;
    str[2] = index[usnum % 16];
    usnum /= 16;
    str[1] = index[usnum % 16];
    usnum /= 16;
    str[0] = index[usnum % 16];
    usnum /= 16;

    return str;
}

#if ( INCLUDE_xTaskGetHandle == 1 )
static int check_task_is_running(void *task)
{
    TaskHandle_t thread = (TaskHandle_t)task;

    if (thread == NULL) {
        return TASK_SUSPEND;
    }

    if (thread == xTaskGetCurrentTaskHandle()) {
        return TASK_RUNNING;
    } else if (thread == xTaskGetCurrentTaskHandle()) {
        return TASK_INTERRUPTED;
    } else if (eTaskGetState(thread) == eReady) {
        return TASK_READY;
    } else {
        return TASK_SUSPEND;
    }
}

static void get_register_from_task_stack(void *context, uint32_t *PC, uint32_t *LR, uint32_t *SP)
{
    XtSolFrame *task_ctx;

    task_ctx = (XtSolFrame *)context;
    *SP = (uint32_t)task_ctx - ALIGNUP(0x10, sizeof(XtSolFrame));
    *PC = task_ctx->pc;
    *LR = *(uint32_t *)(*SP + 16);
}
#endif

static int _backtrace(char *taskname, void *output[], int size, int offset, print_function print_func,
                      void *f, unsigned long exception_mode)
{
    int level = 0;
    char backtrace_output_buf[] = "0x         \r\n";
    backtrace_frame_t stk_frame = {0};
    int depth = 100;
    int check_self = 0;

    if (output && size > 0)
    {
        memset(output, 0, size * sizeof(void *));
    }

    if (taskname)
    {
#if ( INCLUDE_xTaskGetHandle == 1 )
        unsigned long pxTopofStack;
        TaskHandle_t task;

        task = xTaskGetHandle(taskname);
        if (task == NULL)
        {
            PRINT_CALL(print_func, "Task not found : %s\r\n", taskname);
            return 0;
        }
        pxTopofStack = *(unsigned long *)task;
        get_register_from_task_stack((void *)pxTopofStack, &(stk_frame.pc), &(stk_frame.next_pc), &(stk_frame.sp));
        if (check_task_is_running(task) == TASK_RUNNING ||
		    check_task_is_running(task) == TASK_INTERRUPTED)
        {
            check_self = 1;
        }
        if( strcmp( pcTaskGetName( NULL ), taskname ) == 0 )
        {
            check_self = 1;
        }
#else
        PRINT_CALL(print_func, "not support\r\n");
#endif
    }

    if ((taskname == NULL && exception_mode != 1) || check_self == 1)
    {
        esp_backtrace_get_start(&(stk_frame.pc), &(stk_frame.sp), &(stk_frame.next_pc));
    }
    else if (exception_mode == 1)
    {
        XtExcFrame *frame = (XtExcFrame *) f;

        stk_frame.pc = frame->pc;
        stk_frame.sp = frame->a1;
        stk_frame.next_pc = frame->a0;
    }

    if (print_func != NULL)
    {
        long2str((long)cpu_process_stack_pc(stk_frame.pc), &backtrace_output_buf[2]);
        print_func(backtrace_output_buf);
    }

    if (output)
    {
        if (level >= offset && level - offset < size)
        {
            output[level - offset] = (void *)cpu_process_stack_pc(stk_frame.pc);
        }
        if (level - offset >= size)
        {
            goto out;
        }
    }
    level++;

    bool corrupted = !(check_stack_ptr_is_sane(stk_frame.sp) &&
                       (check_ptr_is_valid((void *)cpu_process_stack_pc(stk_frame.pc))));

    uint32_t i = ((depth <= 0) ? INT32_MAX : depth) - 1;
    while (i-- > 0 && stk_frame.next_pc != 0 && !corrupted)
    {
        if (!backtrace_get_next_frame(&stk_frame))
        {
            corrupted = true;
        }
        if (print_func != NULL)
        {
            long2str((long)cpu_process_stack_pc(stk_frame.pc), &backtrace_output_buf[2]);
            print_func(backtrace_output_buf);
        }

        if (output)
        {
            if (level >= offset && level - offset < size)
            {
                output[level - offset] = (void *)cpu_process_stack_pc(stk_frame.pc);
            }
            if (level - offset >= size)
            {
                goto out;
            }
        }
        level++;
    }

    if (corrupted && print_func && stk_frame.pc != 0)
    {
        print_func("invalid pc\r\n");
    }

out:
    return level - offset < 0 ? 0 : level - offset;
}

int arch_backtrace(char *taskname, void *trace[], int size, int offset, print_function print_func)
{
    return _backtrace(taskname, trace, size, offset, print_func, NULL, 0);

}

int arch_backtrace_exception(print_function print_func,
                             void *frame)
{
    return _backtrace(NULL, NULL, 0, 0, print_func, frame, 1);
}
