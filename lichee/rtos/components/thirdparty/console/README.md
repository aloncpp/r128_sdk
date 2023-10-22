# 如何添加command
目前使用的控制台从FreeRTOS社区移植而来,功能较为有限,只能提供一些简单命令,用于系统调试和启动一些应用.可以通过以下两种方法来添加命令.
## 使用FreeRTOS_CLIRegisterCommand接口
该接口由FreeRTOS社区提供，社区原生支持该接口。
### 范例
'''
// pcWriteBuffer为向控制台输出的缓冲区
// xWriteBufferLen 可向控制台缓冲区输出的最大长度
// pcCommandString是命令语句
static portBASE_TYPE prvParameterEchoCommand( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
}

static const CLI_Command_Definition_t xParameterEcho =
{
	"echo-parameters",
	"\r\necho-parameters <...>:\r\n Take variable number of parameters, echos each in turn\r\n",
	prvParameterEchoCommand, /* The function to run. */
	-1 /* The user can enter any number of commands. */
};

FreeRTOS_CLIRegisterCommand( &xParameterEcho );
'''
## 使用FINSH_FUNCTION_EXPORT_CMD宏来定义
FINSH_FUNCTION_EXPORT_CMD从RT-Thread社区的finsh shell移植而来，将命令的名称和入口地址放在指定的段内，在需要时搜索制定的段去执行命令，同时，将命令的入口函数声明为int xxx(int arc, char ** argv);符合大家的编程习惯。
### 范例
'''
#include "console.h"

int cmd_sample(int argc, char ** argv)
{
    int i = 0;
    while(argc--)
    {
	printf("argv[%d] = %s\n", i, argv[i]);
	i++;
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sample, sample, Console Sample Command);
//如此便可添加 sample 命令
'''
