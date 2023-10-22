#include <stdio.h>
#include <console.h>
#include <FreeRTOS.h>
#include "task.h"
#include <FreeRTOS_CLI.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>


#ifdef CONFIG_FREERTOS_CLI_CMD_TASK_STATUS
#define BUF_SIZE 1024
int cmd_ts(int argc, char **argv)
{
	char *const pcHeader = "Task          State  Priority  Stack  #\r\n************************************************\r\n";
	char pcWriteBuffer[BUF_SIZE] = {0};

	/* Generate a table of task stats. */
	strcpy( pcWriteBuffer, pcHeader );
	vTaskList( pcWriteBuffer + strlen( pcHeader ) );
	printf("%s", pcWriteBuffer);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_ts, ts, Display a table showing the state of each FreeRTOS task);

#if 0
#if ( configGENERATE_RUN_TIME_STATS == 1 )
int cmd_run_time_stats(int argc, char **argv)
{
	char *const pcHeader = "Task            Abs Time      % Time\r\n****************************************\r\n";
	char pcWriteBuffer[BUF_SIZE] = {0};

	/* Generate a table of task stats. */
	strcpy( pcWriteBuffer, pcHeader );
	vTaskGetRunTimeStats( ( char * ) pcWriteBuffer + strlen( pcHeader ) );
	printf("%s", pcWriteBuffer);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_run_time_stats, run_time_stats, Display processing time each FreeRTOS task has used);
#endif
#endif

#endif /* CONFIG_FREERTOS_CLI_CMD_TASK_STATUS */

static int cmd_help(int argc, char ** argv)
{
    printf("Lists all the registered commands\n");
    printf("\n");

#ifdef CONFIG_COMPONENTS_FREERTOS_CLI
    typedef struct xCOMMAND_INPUT_LIST
    {
        const CLI_Command_Definition_t *pxCommandLineDefinition;
        struct xCOMMAND_INPUT_LIST *pxNext;
    } CLI_Definition_List_Item_t;

    extern CLI_Definition_List_Item_t xRegisteredCommands;
    CLI_Definition_List_Item_t * pxCommand = &xRegisteredCommands;
    while(pxCommand != NULL)
    {
        printf("[%20s]--------------%s\n",
            pxCommand->pxCommandLineDefinition->pcCommand,
            pxCommand->pxCommandLineDefinition->pcHelpString);
        printf("\n");
        pxCommand = pxCommand->pxNext;
    }
#endif
    finsh_syscall_show();

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_help, help, List all registered commands);

#if 0
int cmd_reg_write(int argc, char ** argv)
{
    uint32_t reg_addr, reg_value ;
    char *err = NULL;

    if(argc < 3)
    {
        printf("Argument Error!\n");
        return -1;
    }

    if ((NULL == argv[1]) || (NULL == argv[2]))
    {
        printf("Argument Error!\n");
        return -1;
    }

    reg_addr = strtoul(argv[1], &err, 0);
    reg_value = strtoul(argv[2], &err, 0);

    if(xport_is_valid_address((void *)reg_addr, NULL))
    {
        *((volatile uint32_t *)(reg_addr)) = reg_value;
    }
    else
    {
        printf("Invalid address!\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_reg_write, reg_write, write value to register: reg_write reg_address reg_value);

int cmd_reg_read(int argc, char ** argv)
{
    uint32_t reg_addr  ;
    char *err = NULL;
    uint32_t start_addr, end_addr ;
    uint32_t len;

    if (NULL == argv[1])
    {
        printf("Argument Error!\n");
        return -1;
    }

    if (argv[2])
    {
        start_addr = strtoul(argv[1], &err, 0);

        len = strtoul(argv[2], &err, 0);
        end_addr = start_addr + len;

        if(xport_is_valid_address((void *)start_addr, (void *)end_addr) && end_addr != 0)
        {
            printf("start_addr=0x%08x end_addr=0x%08x\n", start_addr, end_addr);
            for (; start_addr <= end_addr;)
            {
                printf("reg_addr[0x%08x]=0x%08x \n", start_addr, *((volatile uint32_t *)(start_addr)));
                start_addr += 4;
            }
        }
        else
        {
            printf("Invalid address!\n");
        }
    }
    else
    {
        reg_addr = strtoul(argv[1], &err, 0);
        if(xport_is_valid_address((void *)reg_addr, NULL))
        {
            printf("reg_addr[0x%08x]=0x%08x \n", reg_addr, *((volatile uint32_t *)(reg_addr)));
        }
        else
        {
            printf("Invalid address!\n");
        }
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_reg_read, reg_read, write value to register: reg_read reg_start_addr len);
#endif
