/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __CONSOLE_H
#define __CONSOLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* console port select */
#if defined(CONFIG_ARCH_SUN50IW11)
#ifdef CONFIG_EVB_DSP1_DEBUG
#define CONSOLE_UART   UART_3
#else
#define CONSOLE_UART   UART_0
#endif
#endif /* CONFIG_ARCH_SUN50IW11 */

#if defined(CONFIG_ARCH_SUN8IW20)
#define CONSOLE_UART	UART_2
#endif /* CONFIG_ARCH_SUN8IW20 */

#if defined(CONFIG_ARCH_SUN55IW3)
#define CONSOLE_UART   UART_0
#endif /* CONFIG_ARCH_SUN55IW3 */

#if defined(CONFIG_ARCH_SUN60IW1)
#define CONSOLE_UART   UART_0
#endif/* CONFIG_ARCH_SUN60IW1 */

#if defined(CONFIG_ARCH_SUN20IW2)
#define CONSOLE_UART   CONFIG_CLI_UART_PORT
#endif

#define UART_UNVALID 0xffff
extern int32_t console_uart;


#define SECTION(x)  __attribute__((section(x)))
#define RT_USED     __attribute__((used))

#define SH_MAX_CMD_ARGS 32
#define SH_MAX_CMD_LEN 256

typedef int (*syscall_func)(int argc, char **argv);

/* system call table */
struct finsh_syscall
{
    const char*     name;       /* the name of system call */
    const char*     desc;       /* description of system call */
    syscall_func    func;      /* the function address of system call */
    unsigned int    reserve;
};
extern struct finsh_syscall *_FSymTab_start, *_FSymTab_end;

#define FINSH_FUNCTION_EXPORT_CMD(name, cmd, desc)                      \
                const char __fsym_##cmd##_name[] SECTION(".rodata") = #cmd;    \
                const char __fsym_##cmd##_desc[] SECTION(".rodata") = #desc;   \
                RT_USED const struct finsh_syscall __fsym_##cmd SECTION(".FSymTab")= \
                {                           \
                    __fsym_##cmd##_name,    \
                    __fsym_##cmd##_desc,    \
                    (syscall_func)&name,    \
		    0x00, \
                };

extern struct finsh_syscall* finsh_syscall_lookup(const char* name);

extern void finsh_syscall_show(void);

extern void vUARTCommandConsoleStart(uint16_t usStackSize, uint32_t uxPriority);

#ifdef __cplusplus
}
#endif

#endif /* __CONSOLE_H */
