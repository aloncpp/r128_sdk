#ifndef _ASM_RISCV_CSR_H
#define _ASM_RISCV_CSR_H

#include "asm.h"
#include "consts.h"

/* Status register flags */
#define SR_SIE      _AC(0x00000002, UL) /* Supervisor Interrupt Enable */
#define SR_SPIE     _AC(0x00000020, UL) /* Previous Supervisor IE */
#define SR_SPP      _AC(0x00000100, UL) /* Previously Supervisor */
#define SR_SUM      _AC(0x00040000, UL) /* Supervisor User Memory Access */

#define SR_MIE      _AC(0x00000008, UL) /* Supervisor Interrupt Enable */
#define SR_MPIE     _AC(0x00000080, UL) /* Previous Supervisor IE */
#define SR_MPP      _AC(0x00001800, UL) /* Previously Supervisor */

#define MR_MPP      _AC(0x00001800, UL) /* Previously Machine */
#define MR_MPIE     _AC(0x00000080, UL) /* Previously Machine */
#define MR_MIE      _AC(0x00000008, UL) /* Previously Machine */

#define SR_FS       _AC(0x00006000, UL) /* Floating-point Status */
#define SR_FS_OFF   _AC(0x00000000, UL)
#define SR_FS_INITIAL   _AC(0x00002000, UL)
#define SR_FS_CLEAN _AC(0x00004000, UL)
#define SR_FS_DIRTY _AC(0x00006000, UL)

#define SR_XS       _AC(0x00018000, UL) /* Extension Status */
#define SR_XS_OFF   _AC(0x00000000, UL)
#define SR_XS_INITIAL   _AC(0x00008000, UL)
#define SR_XS_CLEAN _AC(0x00010000, UL)
#define SR_XS_DIRTY _AC(0x00018000, UL)

#define SR_VS       _AC(0x01800000, UL) /* Vecotor Status */
#define SR_VS_OFF   _AC(0x00000000, UL)
#define SR_VS_INITIAL   _AC(0x00800000, UL)
#define SR_VS_CLEAN _AC(0x01000000, UL)
#define SR_VS_DIRTY _AC(0x01800000, UL)


#ifndef CONFIG_64BIT
#define SR_SD       _AC(0x80000000, UL) /* FS/XS dirty */
#else
#define SR_SD       _AC(0x8000000000000000, UL) /* FS/XS dirty */
#endif

/* SATP flags */
#ifndef CONFIG_64BIT
#define SATP_PPN    _AC(0x003FFFFF, UL)
#define SATP_MODE_32    _AC(0x80000000, UL)
#define SATP_MODE   SATP_MODE_32
#else
#define SATP_PPN    _AC(0x00000FFFFFFFFFFF, UL)
#define SATP_MODE_39    _AC(0x8000000000000000, UL)
#define SATP_MODE   SATP_MODE_39
#define SATP_ASID_BITS  16
#define SATP_ASID_SHIFT 44
#define SATP_ASID_MASK  _AC(0xFFFF, UL)
#endif

/* SCAUSE */
#define SCAUSE_IRQ_FLAG     (_AC(1, UL) << (__riscv_xlen - 1))

#define MCAUSE_IRQ_MASK     (_AC(0xfff, UL))
#define IRQ_U_SOFT      0
#define IRQ_S_SOFT      1
#define IRQ_M_SOFT      3
#define IRQ_U_TIMER     4
#define IRQ_S_TIMER     5
#define IRQ_M_TIMER     7
#define IRQ_U_EXT       8
#define IRQ_S_EXT       9
#define IRQ_M_EXT       11
#define IRQ_S_PMU       17

#define EXC_INST_MISALIGNED     0
#define EXC_INST_ACCESS         1
#define EXC_INST_ILLEGAL        2
#define EXC_BREAKPOINT          3
#define EXC_LOAD_MISALIGN       4
#define EXC_LOAD_ACCESS         5
#define EXC_STORE_MISALIGN      6
#define EXC_STORE_ACCESS        7
#define EXC_SYSCALL_FRM_U       8
#define EXC_SYSCALL_FRM_S       9
#define EXC_SYSCALL_FRM_M       11
#define EXC_INST_PAGE_FAULT     12
#define EXC_LOAD_PAGE_FAULT     13
#define EXC_STORE_PAGE_FAULT    15

#define CSR_CYCLE       0xc00
#define CSR_TIME        0xc01
#define CSR_INSTRET     0xc02
#define CSR_CYCLEH      0xc80
#define CSR_TIMEH       0xc81
#define CSR_INSTRETH    0xc82

/* S-Mode System Registers */
/* SIE (Interrupt Enable) and SIP (Interrupt Pending) flags */
#define SIE_SSIE        (_AC(0x1, UL) << IRQ_S_SOFT)
#define SIE_STIE        (_AC(0x1, UL) << IRQ_S_TIMER)
#define SIE_SEIE        (_AC(0x1, UL) << IRQ_S_EXT)
#define SIE_SMIE        (_AC(0x1, UL) << IRQ_S_PMU)

#define CSR_SSTATUS     0x100
#define CSR_SIE         0x104
#define CSR_STVEC       0x105
#define CSR_SCOUNTEREN  0x106

#define CSR_SSCRATCH    0x140
#define CSR_SEPC        0x141
#define CSR_SCAUSE      0x142
#define CSR_STVAL       0x143
#define CSR_SIP         0x144
#define CSR_SATP        0x180

/* M-Mode System Registers */
#define MIE_MSIE        (_AC(0x1, UL) << IRQ_M_SOFT)
#define MIE_MTIE        (_AC(0x1, UL) << IRQ_M_TIMER)
#define MIE_MEIE        (_AC(0x1, UL) << IRQ_M_EXT)

#define CSR_MSTATUS     0x300
#define CSR_MISA        0x301
#define CSR_MEDELEG     0x302
#define CSR_MIDELEG     0x303
#define CSR_MIE         0x304
#define CSR_MTVEC       0x305
#define CSR_MCOUNTEREN  0x306

#define CSR_MSCRATCH    0x340
#define CSR_MEPC        0x341
#define CSR_MCAUSE      0x342
#define CSR_MTVAL       0x343
#define CSR_MIP         0x344

#define CSR_MCOR         0x7c2
#define CSR_MHCR         0x7c1
#define CSR_MCCR2        0x7c3
#define CSR_MHINT        0x7c5
#define CSR_MXSTATUS     0x7c0
#define CSR_PLIC_BASE    0xfc1
#define CSR_MRMR         0x7c6
#define CSR_MRVBR        0x7c7

#define CSR_MCYCLE			0xb00
#define CSR_MINSTRET			0xb02
#define CSR_MHPMCOUNTER3		0xb03
#define CSR_MHPMCOUNTER4		0xb04
#define CSR_MHPMCOUNTER5		0xb05
#define CSR_MHPMCOUNTER6		0xb06
#define CSR_MHPMCOUNTER7		0xb07
#define CSR_MHPMCOUNTER8		0xb08
#define CSR_MHPMCOUNTER9		0xb09
#define CSR_MHPMCOUNTER10		0xb0a
#define CSR_MHPMCOUNTER11		0xb0b
#define CSR_MHPMCOUNTER12		0xb0c
#define CSR_MHPMCOUNTER13		0xb0d
#define CSR_MHPMCOUNTER14		0xb0e
#define CSR_MHPMCOUNTER15		0xb0f
#define CSR_MHPMCOUNTER16		0xb10
#define CSR_MHPMCOUNTER17		0xb11
#define CSR_MHPMCOUNTER18		0xb12
#define CSR_MHPMCOUNTER19		0xb13
#define CSR_MHPMCOUNTER20		0xb14
#define CSR_MHPMCOUNTER21		0xb15
#define CSR_MHPMCOUNTER22		0xb16
#define CSR_MHPMCOUNTER23		0xb17
#define CSR_MHPMCOUNTER24		0xb18
#define CSR_MHPMCOUNTER25		0xb19
#define CSR_MHPMCOUNTER26		0xb1a
#define CSR_MHPMCOUNTER27		0xb1b
#define CSR_MHPMCOUNTER28		0xb1c
#define CSR_MHPMCOUNTER29		0xb1d
#define CSR_MHPMCOUNTER30		0xb1e
#define CSR_MHPMCOUNTER31		0xb1f
#define CSR_MHPMEVENT3			0x323
#define CSR_MHPMEVENT4			0x324
#define CSR_MHPMEVENT5			0x325
#define CSR_MHPMEVENT6			0x326
#define CSR_MHPMEVENT7			0x327
#define CSR_MHPMEVENT8			0x328
#define CSR_MHPMEVENT9			0x329
#define CSR_MHPMEVENT10			0x32a
#define CSR_MHPMEVENT11			0x32b
#define CSR_MHPMEVENT12			0x32c
#define CSR_MHPMEVENT13			0x32d
#define CSR_MHPMEVENT14			0x32e
#define CSR_MHPMEVENT15			0x32f
#define CSR_MHPMEVENT16			0x330
#define CSR_MHPMEVENT17			0x331
#define CSR_MHPMEVENT18			0x332
#define CSR_MHPMEVENT19			0x333
#define CSR_MHPMEVENT20			0x334
#define CSR_MHPMEVENT21			0x335
#define CSR_MHPMEVENT22			0x336
#define CSR_MHPMEVENT23			0x337
#define CSR_MHPMEVENT24			0x338
#define CSR_MHPMEVENT25			0x339
#define CSR_MHPMEVENT26			0x33a
#define CSR_MHPMEVENT27			0x33b
#define CSR_MHPMEVENT28			0x33c
#define CSR_MHPMEVENT29			0x33d
#define CSR_MHPMEVENT30			0x33e
#define CSR_MHPMEVENT31			0x33f
#define CSR_MVENDORID			0xf11
#define CSR_MARCHID			0xf12
#define CSR_MIMPID			0xf13
#define CSR_MHARTID			0xf14
#define CSR_CYCLEH			0xc80
#define CSR_TIMEH			0xc81
#define CSR_INSTRETH			0xc82
#define CSR_HPMCOUNTER3H		0xc83
#define CSR_HPMCOUNTER4H		0xc84
#define CSR_HPMCOUNTER5H		0xc85
#define CSR_HPMCOUNTER6H		0xc86
#define CSR_HPMCOUNTER7H		0xc87
#define CSR_HPMCOUNTER8H		0xc88
#define CSR_HPMCOUNTER9H		0xc89
#define CSR_HPMCOUNTER10H		0xc8a
#define CSR_HPMCOUNTER11H		0xc8b
#define CSR_HPMCOUNTER12H		0xc8c
#define CSR_HPMCOUNTER13H		0xc8d
#define CSR_HPMCOUNTER14H		0xc8e
#define CSR_HPMCOUNTER15H		0xc8f
#define CSR_HPMCOUNTER16H		0xc90
#define CSR_HPMCOUNTER17H		0xc91
#define CSR_HPMCOUNTER18H		0xc92
#define CSR_HPMCOUNTER19H		0xc93
#define CSR_HPMCOUNTER20H		0xc94
#define CSR_HPMCOUNTER21H		0xc95
#define CSR_HPMCOUNTER22H		0xc96
#define CSR_HPMCOUNTER23H		0xc97
#define CSR_HPMCOUNTER24H		0xc98
#define CSR_HPMCOUNTER25H		0xc99
#define CSR_HPMCOUNTER26H		0xc9a
#define CSR_HPMCOUNTER27H		0xc9b
#define CSR_HPMCOUNTER28H		0xc9c
#define CSR_HPMCOUNTER29H		0xc9d
#define CSR_HPMCOUNTER30H		0xc9e
#define CSR_HPMCOUNTER31H		0xc9f
#define CSR_MCYCLEH			0xb80
#define CSR_MINSTRETH			0xb82
#define CSR_MHPMCOUNTER3H		0xb83
#define CSR_MHPMCOUNTER4H		0xb84
#define CSR_MHPMCOUNTER5H		0xb85
#define CSR_MHPMCOUNTER6H		0xb86
#define CSR_MHPMCOUNTER7H		0xb87
#define CSR_MHPMCOUNTER8H		0xb88
#define CSR_MHPMCOUNTER9H		0xb89
#define CSR_MHPMCOUNTER10H		0xb8a
#define CSR_MHPMCOUNTER11H		0xb8b
#define CSR_MHPMCOUNTER12H		0xb8c
#define CSR_MHPMCOUNTER13H		0xb8d
#define CSR_MHPMCOUNTER14H		0xb8e
#define CSR_MHPMCOUNTER15H		0xb8f
#define CSR_MHPMCOUNTER16H		0xb90
#define CSR_MHPMCOUNTER17H		0xb91
#define CSR_MHPMCOUNTER18H		0xb92
#define CSR_MHPMCOUNTER19H		0xb93
#define CSR_MHPMCOUNTER20H		0xb94
#define CSR_MHPMCOUNTER21H		0xb95
#define CSR_MHPMCOUNTER22H		0xb96
#define CSR_MHPMCOUNTER23H		0xb97
#define CSR_MHPMCOUNTER24H		0xb98
#define CSR_MHPMCOUNTER25H		0xb99
#define CSR_MHPMCOUNTER26H		0xb9a
#define CSR_MHPMCOUNTER27H		0xb9b
#define CSR_MHPMCOUNTER28H		0xb9c
#define CSR_MHPMCOUNTER29H		0xb9d
#define CSR_MHPMCOUNTER30H		0xb9e
#define CSR_MHPMCOUNTER31H		0xb9f

#ifndef __ASSEMBLY__

#define csr_swap(csr, val)                  \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrw %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_read(csr)                       \
    ({                              \
        register unsigned long __v;             \
        __asm__ __volatile__ ("csrr %0, " __ASM_STR(csr)    \
                              : "=r" (__v) :            \
                              : "memory");          \
        __v;                            \
    })

#define csr_write(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrw " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define csr_read_set(csr, val)                  \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrs %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_set(csr, val)                   \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrs " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#define csr_read_clear(csr, val)                \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrrc %0, " __ASM_STR(csr) ", %1"\
                              : "=r" (__v) : "rK" (__v)     \
                              : "memory");          \
        __v;                            \
    })

#define csr_clear(csr, val)                 \
    ({                              \
        unsigned long __v = (unsigned long)(val);       \
        __asm__ __volatile__ ("csrc " __ASM_STR(csr) ", %0" \
                              : : "rK" (__v)            \
                              : "memory");          \
    })

#endif /* __ASSEMBLY__ */

#endif /* _ASM_RISCV_CSR_H */
