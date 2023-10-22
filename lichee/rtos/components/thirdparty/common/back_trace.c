#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <task.h>
#include <portable.h>
#include <string.h>
#include <console.h>
#include <context.h>

#include <backtrace.h>

int backtrace_exception(print_function print_func, unsigned long cpsr,
                        unsigned long sp,
                        unsigned long pc,
                        unsigned long lr)
{
#ifdef CONFIG_DEBUG_BACKTRACE
    return arch_backtrace_exception(print_func, cpsr, sp, pc, lr);
#else
    if (print_func)
    {
        print_func("not support backtrace!\n");
    }
    return 0;
#endif
}

/*
 * backtrace: get call stack information.
 * @taskname: target task. NULL: current task.
 * @output: call stack output buffer.
 * @offset:
 * @print_func: print function
 */
int backtrace(char *taskname, void *output[], int size, int offset, print_function print_func)
{
#ifdef CONFIG_DEBUG_BACKTRACE
    return arch_backtrace(taskname, output, size, offset, print_func);
#else
    if (print_func)
    {
        print_func("not support backtrace!\n");
    }
    return 0;
#endif
}

#ifdef CONFIG_DEBUG_BACKTRACE_FRAME_POINTER
extern void dump_stack(void *fp);

static void backtrace_help()
{
    printf("Usage: backtrace [task_name]\n");
}

static int backtrace_frame_pointer(int argc, char ** argv)
{
    int ret = 0;
    char *task_name = NULL;
    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x;
    switch_ctx_regs_t *regs_ctx;
    TaskStatus_t temp;
    uint32_t cpsr_flag;
    uint32_t *fp = NULL;
    TaskHandle_t clitask = NULL;

    if (argc > 2) {
        backtrace_help();
        return -1;
    } else if (argc == 2) {
        task_name = argv[1];
    }

    taskENTER_CRITICAL(cpsr_flag);
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    if (!pxTaskStatusArray) {
        printf("malloc %d bytes error!\n", uxArraySize * sizeof(TaskStatus_t));
        return -1;
    }
    memset(pxTaskStatusArray, 0, uxArraySize * sizeof( TaskStatus_t  ));

    uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, NULL );

    clitask = xTaskGetCurrentTaskHandle();

    for (x = 0; x < uxArraySize; x++) {
        temp = pxTaskStatusArray[x];

        TaskHandle_t core_task = xTaskGetCurrentTaskHandleOnCore(temp.bind_cpu);
        if(core_task == temp.xHandle && core_task != clitask){
            if(task_name && !strcmp(task_name, pcTaskGetName(core_task))){
                printf("Task[%s] is running on core%d now, can not be backtraced!\n", temp.pcTaskName, temp.bind_cpu);
            }
            else if(task_name == NULL){
                printf("Task[%s] is running on core%d now, can not be backtraced!\n", temp.pcTaskName, temp.bind_cpu);
            }
            continue;
        }

        if (task_name) {
            if (!strcmp(task_name, temp.pcTaskName)) {
                printf("==> Task: %s\n", temp.pcTaskName);
                if (clitask == temp.xHandle) {
                    dump_stack(NULL);
                    break;
		} else {
                    regs_ctx = (switch_ctx_regs_t *)(temp.pxTopOfStack);
                    if ((regs_ctx->r11 > (uint32_t)temp.pxStackBase) && (regs_ctx->r11 < (uint32_t)temp.pxStackBase + temp.pxStackSize)) {
                        dump_stack((uint32_t *)regs_ctx->r11);
                    } else
                        printf("Invalid fp address: 0x%x\n", regs_ctx->r11);
                }
            }
        } else {
            if (!temp.pcTaskName)
                continue;

            printf("==> Task: %s\n", temp.pcTaskName);
            if (clitask == temp.xHandle) {
                dump_stack(NULL);
            } else {
                regs_ctx = (switch_ctx_regs_t *)(temp.pxTopOfStack);
                if ((regs_ctx->r11 > (uint32_t)temp.pxStackBase) && (regs_ctx->r11 < (uint32_t)temp.pxStackBase + temp.pxStackSize)) {
                    dump_stack((uint32_t *)regs_ctx->r11);
                } else
                    printf("Invalid fp address: 0x%x\n", regs_ctx->r11);
            }
        }
    }
    vPortFree(pxTaskStatusArray);
    taskEXIT_CRITICAL(cpsr_flag);
}
#endif

int cmd_backtrace(int argc, char ** argv)
{
    printf("Usage : backtrace [taskname | tasknumber]\r\n");
#ifdef CONFIG_DEBUG_BACKTRACE
    if (argc < 2)
        backtrace(NULL, NULL, 0, 0, printf);
    else
        backtrace(argv[1], NULL, 0, 0, printf);
#elif CONFIG_DEBUG_BACKTRACE_FRAME_POINTER
    backtrace_frame_pointer(argc, argv);
#endif
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_backtrace, backtrace, Backtrace Command);
