/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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
#include <stdint.h>
#include <stdio.h>
#include <hal_interrupt.h>
#include <interrupt.h>
#include <excep.h>
#include <inttypes.h>
#include <sunxi_hal_common.h>
#include <irqflags.h>

#include "csi_rv32_gcc.h"
#include "soc.h"
#include "core_rv32.h"

#include "platform/platform.h"

extern void Default_Handler(void);
extern void enter_interrupt_handler(void);
extern void exit_interrupt_handler(void);
int32_t drv_get_sys_freq(void);

#define get_major(x)            ((x) / 100)
#define get_minor(x)              ((x) % 100)

/* The real is 109, in order to align to 8bytes,we adjust to 110  */
void (*g_irqvector[CLIC_IRQ_NUM + 1])(void);
void (*g_nmivector)(void);
#ifdef CONFIG_STANDBY
uint8_t g_irq_status[CLIC_IRQ_NUM] = {0};
#endif

struct arch_irq_desc {
    hal_irq_handler_t handle_irq;
    void *data;
};

static struct arch_irq_desc arch_irqs_desc[CLIC_IRQ_NUM - CLIC_PERIPH_IRQ_OFFSET - CLIC_IRQ_GROUP_NUM];
/* only support one interrupt per group */
static struct arch_irq_desc arch_irq_group_desc[CLIC_IRQ_GROUP_NUM][CLIC_IRQ_PER_GROUP];

void SysTick_Handler(void)
{
    enter_interrupt_handler();
    csi_coret_config(drv_get_sys_freq() / CONFIG_HZ, CORET_IRQn);
    xTaskIncrementTick();
    exit_interrupt_handler();
}

void riscv_timer_interrupt(void)
{
    csi_coret_config(drv_get_sys_freq() / CONFIG_HZ, CORET_IRQn);
}

static int is_enable_subirq(unsigned int major, unsigned int sub);
void show_irqs(void)
{
    int i, j;
    int enable;
    const char *irq_name;
    const char *irq_status;
    printf("IRQ    Status    Name\r\n");
    for (i = 0; i < CLIC_IRQ_GROUP_OFFSET; i++) {
        if (csi_vic_get_enabled_irq(i)) {
            irq_status = "Enabled ";
        } else {
            irq_status = "Disabled";
        }

        irq_name = irq_major_string[i];
        if (irq_name) {
            printf("%3d    %s   %s\r\n", i, irq_status, irq_name);
        }
    }

    for (i = 0; i < (CLIC_IRQ_GROUP_NUM); i++) {
        irq_name = irq_major_string[i + CLIC_IRQ_GROUP_OFFSET];
        if (csi_vic_get_enabled_irq(i + CLIC_IRQ_GROUP_OFFSET))
            irq_status = "Enabled ";
        else
            irq_status = "Disabled";

        printf("%3d    %s   %s\r\n", i + CLIC_IRQ_GROUP_OFFSET, irq_status, irq_name);
        for (j = 0; j < (CLIC_IRQ_PER_GROUP); j++) {
            if (is_enable_subirq(i + CLIC_IRQ_GROUP_OFFSET, j + 1)) {
                irq_status = "Enabled ";
            } else {
                irq_status = "Disabled";
            }

            irq_name = irq_group_string[i * CLIC_IRQ_PER_GROUP + j];
            if (irq_name) {
                printf("    %3d    %s   %s\r\n", j + 1, irq_status, irq_name);
            }
        }
    }

}

static hal_irqreturn_t clic_null_handler(void *data)
{
    return 0;
}

unsigned long arch_irq_is_disable(void)
{
    return arch_irqs_disabled_flags(arch_local_save_flags());
}

static void enable_irq(unsigned int irq_num)
{
    if (NMI_EXPn != irq_num) {
        csi_vic_enable_irq(irq_num);
#ifdef CONFIG_STANDBY
        g_irq_status[irq_num] = 1;
#endif
    }
}

static void disable_irq(unsigned int irq_num)
{
    if (NMI_EXPn != irq_num) {
        csi_vic_disable_irq(irq_num);
#ifdef CONFIG_STANDBY
        g_irq_status[irq_num] = 0;
#endif
    }
}

/**
 * Enabled a sub-interrupt number
 * @major: major interrupt number
 * @sub:   sub interrupt, start with 1.
 * */
static void enable_subirq(unsigned int major, unsigned int sub)
{
    uint32_t group, group_index, bit, reg, val;

    group = (major - CLIC_IRQ_GROUP_OFFSET) / 4;
    group_index = (major - CLIC_IRQ_GROUP_OFFSET) % 4;

    if (group >= CLIC_IRQ_GROUP_NUM) {
        printf("Invalid major-interrupt number, must be [%d .. %d]",
               CLIC_IRQ_GROUP_OFFSET, CLIC_IRQ_GROUP_OFFSET + CLIC_IRQ_GROUP_NUM);
        return;
    }
    if (sub == 0 || sub > CLIC_IRQ_PER_GROUP) {
        printf("Invalid Sub-interrupt number, must be [1 .. %d]", CLIC_IRQ_PER_GROUP);
        return;
    }

    bit = (1 << (sub - 1)) << (group_index * 8);

    val = hal_readl(GROUP_ENABLE_REG(group));
    val |= bit;
    hal_writel(val, GROUP_ENABLE_REG(group));
}

static void disable_subirq(unsigned int major, unsigned int sub)
{
    uint32_t group, group_index, bit, reg, val;

    group = (major - CLIC_IRQ_GROUP_OFFSET) / 4;
    group_index = (major - CLIC_IRQ_GROUP_OFFSET) % 4;

    if (group >= CLIC_IRQ_GROUP_NUM) {
        printf("Invalid major-interrupt number, must be [%d .. %d]",
               CLIC_IRQ_GROUP_OFFSET, CLIC_IRQ_GROUP_OFFSET + CLIC_IRQ_GROUP_NUM);
        return;
    }
    if (sub == 0 || sub > CLIC_IRQ_PER_GROUP) {
        printf("Invalid Sub-interrupt number, must be [1 .. %d]", CLIC_IRQ_PER_GROUP);
        return;
    }

    bit = (1 << (sub - 1)) << (group_index * 8);

    val = hal_readl(GROUP_ENABLE_REG(group));
    val &= ~(bit);
    hal_writel(val, GROUP_ENABLE_REG(group));
}

static int is_enable_subirq(unsigned int major, unsigned int sub)
{
    uint32_t group, group_index, bit, reg, val;

    group = (major - CLIC_IRQ_GROUP_OFFSET) / 4;
    group_index = (major - CLIC_IRQ_GROUP_OFFSET) % 4;

    if (group >= CLIC_IRQ_GROUP_NUM) {
        printf("Invalid major-interrupt number %d, must be [%d .. %d]",
               group, CLIC_IRQ_GROUP_OFFSET, CLIC_IRQ_GROUP_OFFSET + CLIC_IRQ_GROUP_NUM);
        return 0;
    }
    if (sub == 0 || sub > CLIC_IRQ_PER_GROUP) {
        printf("Invalid Sub-interrupt number %d, must be [1 .. %d]", sub, CLIC_IRQ_PER_GROUP);
        return 0;
    }

    bit = (1 << (sub - 1)) << (group_index * 8);

    val = hal_readl(GROUP_ENABLE_REG(group));
    if (val & bit) {
        return 1;
    } else {
        return 0;
    }
}

void arch_enable_irq(unsigned int irq_num)
{
    int major = get_major(irq_num);
    int subirq = get_minor(irq_num);

    if (major >= CLIC_IRQ_GROUP_OFFSET && !subirq) {
        printf("Warnning: %d is interrupt group, "
               "plese specify the sub-interrupt num.\r\n", irq_num);
        return;
    }

    enable_irq(major);

    if (major >= CLIC_IRQ_GROUP_OFFSET) {
        enable_subirq(major, subirq);
    }
}

void arch_disable_irq(unsigned int irq_num)
{
    int major = get_major(irq_num);
    int subirq = get_minor(irq_num);

    if (major >= CLIC_IRQ_GROUP_OFFSET && !subirq) {
        printf("Warnning: %d is interrupt group, "
               "plese specify the sub-interrupt num.\r\n", irq_num);
        return;
    }

    disable_irq(major);

    if (major >= CLIC_IRQ_GROUP_OFFSET) {
        disable_subirq(major, subirq);
    }
}

void xport_interrupt_enable(unsigned long flags)
{
    return arch_local_irq_restore(flags);
}

unsigned long xport_interrupt_disable(void)
{
    return arch_local_irq_save();
}

void arch_disable_all_irq(void)
{
    arch_local_irq_disable();
}

void arch_enable_all_irq(void)
{
    arch_local_irq_enable();
}

#ifdef CONFIG_STANDBY
void irq_suspend(void)
{
    int i = 0;

    for (i = 0; i < CLIC_IRQ_NUM; i++) {
        if (g_irq_status[i]) {
            csi_vic_disable_irq(i);
        }
    }
}

void irq_resume(void)
{
    int i = 0;

    for (i = 0; i < CLIC_IRQ_NUM; i++) {
        if (g_irq_status[i]) {
            csi_vic_enable_irq(i);
        }
    }
}
#endif

int32_t arch_request_irq(int32_t irq, hal_irq_handler_t handler, void *data)
{
    uint32_t major = get_major(irq);
    uint32_t minor = get_minor(irq);

    if (major < CLIC_PERIPH_IRQ_OFFSET) {
        g_irqvector[major] = (void *)handler;
        return major;
    }

    if (major < CLIC_IRQ_GROUP_OFFSET) {
        if (handler && arch_irqs_desc[major - CLIC_PERIPH_IRQ_OFFSET].handle_irq == (void *)clic_null_handler) {
            arch_irqs_desc[major - CLIC_PERIPH_IRQ_OFFSET].handle_irq = (void *)handler;
            arch_irqs_desc[major - CLIC_PERIPH_IRQ_OFFSET].data = data;
        }
        return major;
    }
    if (major < CLIC_IRQ_NUM) {
        if (handler && arch_irq_group_desc[major - CLIC_IRQ_GROUP_OFFSET][minor].handle_irq == (void *)clic_null_handler) {
            arch_irq_group_desc[major - CLIC_IRQ_GROUP_OFFSET][minor].handle_irq = (void *)handler;
            arch_irq_group_desc[major - CLIC_IRQ_GROUP_OFFSET][minor].data = data;
        }
        return major;
    }

    printf("Wrong irq NO.(%"PRIu32") to request !!\n", major);
    return -1;
}

void arch_free_irq(uint32_t irq)
{
    uint32_t major = get_major(irq);
    uint32_t minor = get_minor(irq);

    if (major < CLIC_PERIPH_IRQ_OFFSET) {
        g_irqvector[major] = (void *)Default_Handler;
        return;
    }
    if (major < CLIC_IRQ_GROUP_OFFSET) {
        arch_irqs_desc[major - CLIC_PERIPH_IRQ_OFFSET].handle_irq = (void *)clic_null_handler;
    }
    if (major < CLIC_IRQ_NUM && (major >= CLIC_IRQ_GROUP_OFFSET)) {
        arch_irq_group_desc[major - CLIC_IRQ_GROUP_OFFSET][minor].handle_irq = (void *)clic_null_handler;
    }
    if (major >= CLIC_IRQ_NUM) {
        printf("Invalid irq number:%d.\n", major);
    }

    return;
}

int request_threaded_irq(unsigned int irq, hal_irq_handler_t handler,
                         hal_irq_handler_t thread_fn, unsigned long irqflags,
                         const char *devname, void *dev_id)
{
    return arch_request_irq(irq, (void *)handler, dev_id);
}

const void *free_irq(int32_t irq)
{
    arch_free_irq(irq);
    return NULL;
}

unsigned long riscv_cpu_handle_interrupt(unsigned long scause, unsigned long sepc, unsigned long stval, irq_regs_t *regs)
{
    printf("E906 will not support the interrupt mode!\n");
    printf("cause:0x%08lx mepc:0x%08lx mtval:0x%08lx\r\n", scause, sepc, stval);
    return 0;
}

void clic_common_handler(void)
{
    uint32_t id;
    uint32_t group, group_index;
    int i;

    enter_interrupt_handler();
    id = (__get_MCAUSE() & 0xfff);

    if (id >= CLIC_IRQ_GROUP_OFFSET) {
        goto interrupt_group;
    }

    id -= CLIC_PERIPH_IRQ_OFFSET;
    if (arch_irqs_desc[id].handle_irq &&
            arch_irqs_desc[id].handle_irq != (void *)clic_null_handler) {
        arch_irqs_desc[id].handle_irq(arch_irqs_desc[id].data);
    } else {
        printf("no handler for irq %d\n", id);
    }

    exit_interrupt_handler();

    return;

interrupt_group:
    id -= CLIC_IRQ_GROUP_OFFSET;
    for(i = 0; i < CLIC_IRQ_PER_GROUP; i++) {
	    if (arch_irq_group_desc[id][i].handle_irq &&
                arch_irq_group_desc[id][i].handle_irq != (void *)clic_null_handler) {
            if (arch_irq_group_desc[id][i].handle_irq(arch_irq_group_desc[id][i].data) >= 0)
                break;
        }
    }
    if (i == CLIC_IRQ_PER_GROUP) {
        printf("no handler for irq %d\n", id);
    }

    exit_interrupt_handler();
}

void irq_vectors_init(void)
{
    int i, j;

    for (i = CLIC_PERIPH_IRQ_OFFSET; i < CLIC_IRQ_GROUP_OFFSET; i++) {
#ifdef CONFIG_STANDBY
        g_irq_status[i] = 0;
#endif
        disable_irq(i);
        g_irqvector[i] = (void *)clic_common_handler;
        arch_irqs_desc[i - CLIC_PERIPH_IRQ_OFFSET].handle_irq = (void *)clic_null_handler;
        arch_irqs_desc[i - CLIC_PERIPH_IRQ_OFFSET].data = NULL;
    }

    for (i = 0; i < (CLIC_IRQ_GROUP_NUM); i++) {
        disable_irq(i + CLIC_IRQ_GROUP_OFFSET);
        g_irqvector[i + CLIC_IRQ_GROUP_OFFSET] = (void *)clic_common_handler;
        for (j = 0; j < (CLIC_IRQ_PER_GROUP); j++) {
            arch_irq_group_desc[i][j].handle_irq = (void *)clic_null_handler;
            arch_irq_group_desc[i][j].data = NULL;
        }
    }

#ifdef CONFIG_STANDBY
    for (i = 0; i < CLIC_PERIPH_IRQ_OFFSET; i++) {
        if (csi_vic_get_enabled_irq(i)) {
            g_irq_status[i] = 1;
        } else {
            g_irq_status[i] = 0;
        }
    }
#endif

    g_irqvector[CORET_IRQn] = SysTick_Handler;
}

