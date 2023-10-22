
#ifndef CONTEXT_H
#define CONTEXT_H

#include <FreeRTOS.h>
#include <stdint.h>


#if (CONFIG_ARCH_ARM_CORTEX_M33 == 1)
typedef struct
{
    uint32_t psplim;
#if ( configENABLE_MPU == 1 )
    uint32_t control;
#endif
    uint32_t r14;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
#if ( configENABLE_FPU == 1 )
    uint32_t fp_reg_h[16]; /* index from 16 to 31 */
#endif
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t retAddr;
    uint32_t xPSR;
#if ( configENABLE_FPU == 1 )
    uint32_t fp_reg_l[16]; /* index from 0 to 15 */
    uint32_t fpscr;
    uint32_t reserves;
#endif

} switch_ctx_regs_t;

#elif (CONFIG_ARCH_RISCV == 1)

typedef struct
{
    unsigned long mepc;
    unsigned long ra;
    unsigned long x5_x31[27];
    unsigned long mstatus;
    unsigned long fp;
    unsigned long gp;
    unsigned long tp;
    unsigned long mscratch;
} switch_ctx_regs_t;

#else
typedef struct
{
    uint32_t ulPortTaskHasFPUContext;
    uint32_t fpscr;
#if (configUSE_TASK_FPU_SUPPORT == 2)
    uint64_t fp_d_register[32];
#endif
    uint32_t ulCriticalNestingConst;
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
    uint32_t r12;
    uint32_t r14;
} switch_ctx_regs_t;

#endif

#endif  /*CONTEXT_H*/
