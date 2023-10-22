#include <stdio.h>
#include "console.h"
#include <FreeRTOS.h>
#include "../FreeRTOS_CLI.h"
#include <task.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

struct fork_arg_t
{
    struct finsh_syscall * syscall;
    int argc;
    char *argv[SH_MAX_CMD_ARGS];
    char * argv_cmd;
};

static int priority = 4;
static uint32_t thread_size = 8 * 1024 * sizeof(StackType_t);

static int cmd_forkarg(int argc, char **argv)
{
    int opts = 0;
    int size = thread_size;
    int prio = priority;
    char *err = NULL;

    optind = 0;
    while ((opts = getopt(argc, argv, ":hs:p:")) != EOF)
    {
        switch (opts)
        {
            case 'h':
                printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                goto out;
            case 's':
            {
                size = strtoul(optarg, &err, 0);
                if (!size)
                {
                    printf("size %s is zero or invalid\n", optarg);
                    printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                    goto out;
                }
                err = NULL;
                break;
            }
            case 'p':
            {
                prio = strtoul(optarg, &err, 0);
                if (!prio)
                {
                    printf("priority %s is zero or invalid\n", optarg);
                    printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                    goto out;
                }
                err = NULL;
                break;
            }
            case '?':
                printf("invalid option %c\n", optopt);
                printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                goto out;
            case ':':
                printf("option -%c requires an argument\n", optopt);
                printf("Usage : forkarg [-p task_priority] [-s stack_size]\n");
                goto out;
            default:
                break;
        }
    }

    if (prio > (configMAX_PRIORITIES - 1) || prio <= 0)
    {
        printf("The priority(%d) out of range[%d-%d]\n", prio, 0, configMAX_PRIORITIES - 1);
        goto out;
    }
    priority = prio;

    if ((size > (USHRT_MAX + 1) * sizeof(StackType_t)) || size <= 0)
    {
        printf("The stack size(%d) out of range[%d-%d] (bytes)\n", size, 0, (USHRT_MAX + 1) * sizeof(StackType_t));
        goto out;
    }
    thread_size = size;

    printf("fork command priority is %d, stack size is %d (bytes)\n", priority, thread_size);
out:
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_forkarg, forkarg, Set form command prority and stack size);

static void print_usage(void)
{
    printf("Unage:fork [command arg0 arg1 ...]\n \
        example:\n \
            fork help\n \
            fork ls -l /data\n \
            fork cd /data\n");
}

static void fork_thread_entry(void * arg)
{
    if(arg == NULL)
    {
        vTaskDelete(NULL);
    }

    struct fork_arg_t * fork_arg = arg;
    struct finsh_syscall * call = fork_arg->syscall;

    call->func(fork_arg->argc, fork_arg->argv);

    if(fork_arg->argv_cmd != NULL)
    {
        free(fork_arg->argv_cmd);
    }

    free(fork_arg);

    vTaskDelete(NULL);
}

int cmd_fork(int argc, char ** argv)
{
    portBASE_TYPE ret;
    struct finsh_syscall* call;
    char * command_name = NULL;
    int i;

    if(argc < 2)
    {
        print_usage();
        return -1;
    }

    command_name = argv[1];

    call = finsh_syscall_lookup(command_name);
    if(call == NULL)
    {
        printf("The command no exist!\n");
        return -1;
    }

    if(call->func == NULL)
    {
        printf("Command entry no exist\n");
        return -1;
    }

    struct fork_arg_t * fork_arg = malloc(sizeof(struct fork_arg_t));
    if(fork_arg == NULL)
    {
        printf("Alloc memory failed!\n");
        return -1;
    }
    memset(fork_arg, 0, sizeof(struct fork_arg_t));

    char * fork_args_cmd = malloc(SH_MAX_CMD_LEN);
    if(fork_args_cmd == NULL)
    {
        printf("Alloc memory failed!\n");
        free(fork_arg);
        return -1;
    }
    memset(fork_args_cmd, 0, SH_MAX_CMD_LEN);

    fork_arg->argv_cmd = fork_args_cmd;
    fork_arg->syscall = call;
    fork_arg->argc = argc - 1;

    for(i = 1; i < argc; i++)
    {
        fork_arg->argv[i - 1] = fork_args_cmd;
        memcpy(fork_args_cmd, argv[i], strlen(argv[i]));
        fork_args_cmd += (strlen(argv[i]) + 1);
    }

    ret = xTaskCreate(fork_thread_entry, call->name, thread_size / sizeof(StackType_t), (void *)fork_arg, priority, NULL);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_fork, fork, Create a task to run: fork command_name arg1 arg2 ...);
