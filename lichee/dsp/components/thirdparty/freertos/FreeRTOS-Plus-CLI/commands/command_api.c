#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "console.h"

#define FINSH_NEXT_SYSCALL(index)  index++
#define FINSH_NEXT_SYSVAR(index)   index++

extern struct finsh_syscall __fsym_help;
struct finsh_syscall* finsh_syscall_lookup(const char* name)
{
    struct finsh_syscall* index;

    /*
     * FSymTab section maybe ignored when link(because of --gc-sections),
     * so make sure the value(__fsym_help, in FSymTab section) in used here.
     **/
    if (!strcmp(name, __fsym_help.name))
        return &__fsym_help;
    for (index = (struct finsh_syscall *)&_FSymTab_start; (uint32_t)index < (uint32_t)&_FSymTab_end; FINSH_NEXT_SYSCALL(index))
    {
        if (strcmp(index->name, name) == 0)
            return index;
    }
    return NULL;
}

void finsh_syscall_show(void)
{
    struct finsh_syscall* index;

    for (index = (struct finsh_syscall *)&_FSymTab_start; (uint32_t)index < (uint32_t)&_FSymTab_end; FINSH_NEXT_SYSCALL(index))
    {
        printf("[%20s]--------------%s\n",
            index->name,
            index->desc);
        printf("\n");
    }
}

enum ParseState
{
    PS_WHITESPACE,
    PS_TOKEN,
    PS_STRING,
    PS_ESCAPE
};

void console_parseargs(char *argstr, int *argc_p, char **argv, char **resid)
{
    int argc = 0;
    char c = 0;
    enum ParseState stackedState = PS_WHITESPACE;
    enum ParseState lastState = PS_WHITESPACE;

    /* tokenize the argstr */
    while ((c = *argstr) != 0)
    {
        enum ParseState newState;

        if (c == ';' && lastState != PS_STRING && lastState != PS_ESCAPE)
        {
            break;
        }

        if (lastState == PS_ESCAPE)
        {
            newState = stackedState;
        }
        else if (lastState == PS_STRING)
        {
            if (c == '"')
            {
                newState = PS_WHITESPACE;
                *argstr = 0;
            }
            else
            {
                newState = PS_STRING;
            }
        }
        else if ((c == ' ') || (c == '\t'))
        {
            /* whitespace character */
            *argstr = 0;
            newState = PS_WHITESPACE;
        }
        else if (c == '"')
        {
            newState = PS_STRING;
            *argstr++ = 0;
            argv[argc++] = argstr;
            /*      } else if (c == '\\') {
                        stackedState = lastState;
                        newState = PS_ESCAPE;   */
        }
        else
        {
            /* token */
            if (lastState == PS_WHITESPACE)
            {
                argv[argc++] = argstr;
            }

            newState = PS_TOKEN;
        }

        lastState = newState;
        argstr++;
    }

    argv[argc] = NULL;

    if (argc_p != NULL)
    {
        *argc_p = argc;
    }

    if (*argstr == ';')
    {
        *argstr++ = '\0';
    }

    *resid = argstr;
}

portBASE_TYPE prvCommandEntry( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
    struct finsh_syscall *cmd_syscall = NULL;
    BaseType_t xReturn = pdFALSE;
    int ret = 0;

    int argc = 0;
    char *argv[SH_MAX_CMD_ARGS];
    char *resid = 0;

    char *buf;
    char temp [SH_MAX_CMD_LEN];
    int i;

    optarg = NULL;
    optind = opterr = optopt = 0;

    memcpy(temp, pcCommandString, SH_MAX_CMD_LEN);
    buf = (char *)&temp;

    for (i = 0; i < SH_MAX_CMD_ARGS; i++)
    {
        argv[i] = NULL;
    }
    console_parseargs(buf, &argc, argv, &resid);

    if(argc <= 0)
    {
        return pdFALSE;
    }

    cmd_syscall = finsh_syscall_lookup(argv[0]);
    if(cmd_syscall && cmd_syscall->func)
    {
        ret = cmd_syscall->func(argc, argv);
        xReturn = pdTRUE;
    }
    else
    {
        xReturn = pdFALSE;
    }
    return xReturn;
}
