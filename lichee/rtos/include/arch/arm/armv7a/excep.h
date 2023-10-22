/*
 * =====================================================================================
 *
 *       Filename:  excep.h
 *
 *    Description:  Exceptions context definition.
 *
 *        Version:  Melis3.0
 *         Create:  2017-11-02 10:16:16
 *       Revision:  none
 *       Compiler:  gcc version 6.3.0 (crosstool-NG crosstool-ng-1.23.0)
 *
 *         Author:  zhijinzeng@allwinnertech.com
 *   Organization:  PDC-PSW
 *  Last Modified:  2020-06-26 11:15:01
 *
 * =====================================================================================
 */

#ifndef  __EXP926_INC__
#define  __EXP926_INC__

#include <stdint.h>
#include <list.h>

typedef struct
{
    uint32_t fcsid;                     // 0x00
    uint32_t abt_iFAR;                  // 0x04
    uint32_t abt_dFAR;                  // 0x08
    uint32_t abt_iFSR;                  // 0x0c
    uint32_t abt_dFSR;                  // 0x10
    uint32_t abt_lr;                    // 0x14
    uint32_t ttbr0;                     // 0x18
    uint32_t ttbr1;                     // 0x1c
    uint32_t sctl;                      // 0x20
    uint32_t scr;                       // 0x24
    uint32_t orig_cpsr;                 // 0x28
}ctrl_regs_t;

typedef struct
{
    ctrl_regs_t control;                //0x00-0x24
    uint32_t lr;                        // 0x28
    uint32_t sp;                        // 0x2c
    uint32_t r0;                        // 0x30
    uint32_t r1;                        // 0x34
    uint32_t r2;                        // 0x38
    uint32_t r3;                        // 0x3c
    uint32_t r4;                        // 0x40
    uint32_t r5;                        // 0x44
    uint32_t r6;                        // 0x48
    uint32_t r7;                        // 0x4c
    uint32_t r8;                        // 0x50
    uint32_t r9;                        // 0x54
    uint32_t r10;                       // 0x58
    uint32_t r11;                       // 0x5c
    uint32_t r12;                       // 0x60
} excep_regs_t;

struct undef_hook
{
    struct list_head node;
    uint32_t instr_mask;
    uint32_t instr_val;
    uint32_t cpsr_mask;
    uint32_t cpsr_val;
    int (*fn)(excep_regs_t *regs);
};

struct data_abort_hook
{
    struct list_head node;
    uint32_t dfsr_mask;
    uint32_t dfsr_val;
    uint32_t dfar_mask;
    uint32_t dfar_val;
    uint32_t cpsr_mask;
    uint32_t cpsr_val;
    int (*fn)(excep_regs_t *regs);
};

struct instr_prefetch_abort_hook
{
    struct list_head node;
    uint32_t ifsr_mask;
    uint32_t ifsr_val;
    uint32_t ifar_mask;
    uint32_t ifar_val;
    uint32_t cpsr_mask;
    uint32_t cpsr_val;
    int (*fn)(excep_regs_t *regs);
};

void register_undef_hook(struct undef_hook *hook);
void unregister_undef_hook(struct undef_hook *hook);

void register_data_abort_hook(struct data_abort_hook *hook);
void unregister_data_abort_hook(struct data_abort_hook *hook);

void register_instr_prefetch_abort_hook(struct instr_prefetch_abort_hook *hook);
void unregister_instr_prefetch_abort_hook(struct instr_prefetch_abort_hook *hook);

#endif
