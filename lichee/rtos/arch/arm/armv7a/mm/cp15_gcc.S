#include <autoconf.h>
.global xport_cpu_mmu_get_ttbr0
xport_cpu_mmu_get_ttbr0:
    MRC     P15, 0, r0, c2, c0, 0   //read TTBR0.
    bx      lr

.global xport_cpu_mmu_get_ttbr1
xport_cpu_mmu_get_ttbr1:
    MRC     P15, 0, r0, c2, c0, 1   //read TTBR1.
    bx      lr

.globl cur_cpu_id
cur_cpu_id:
#ifdef CONFIG_SMP
    mrc     p15, #0, r0, c0, c0, #5
    and     r0, r0, #0x3
#else
    mov     r0, #0
#endif
    bx      lr

.globl xport_cpu_vector_set_base
xport_cpu_vector_set_base:
    /* clear SCTRL.V to customize the vector address */
    mrc     p15, #0, r1, c1, c0, #0
    bic     r1, #(1 << 13)
    mcr     p15, #0, r1, c1, c0, #0
    /* set up the vector address */
    mcr     p15, #0, r0, c12, c0, #0
    dsb
    bx      lr

.globl xport_cpu_dcache_enable
xport_cpu_dcache_enable:
    mrc     p15, #0, r0, c1, c0, #0
    orr     r0,  r0, #0x00000004
    mcr     p15, #0, r0, c1, c0, #0
    bx      lr

.globl xport_cpu_icache_enable
xport_cpu_icache_enable:
    mrc     p15, #0, r0, c1, c0, #0
    orr     r0,  r0, #0x00001000
    mcr     p15, #0, r0, c1, c0, #0
    bx      lr

_FLD_MAX_WAY:
   .word  0x3ff
_FLD_MAX_IDX:
   .word  0x7fff

.globl xport_cpu_dcache_clean_flush
xport_cpu_dcache_clean_flush:
    push    {r4-r11}
    dmb
    mrc     p15, #1, r0, c0, c0, #1  @ read clid register
    ands    r3, r0, #0x7000000       @ get level of coherency
    mov     r3, r3, lsr #23
    beq     finished
    mov     r10, #0
loop1:
    add     r2, r10, r10, lsr #1
    mov     r1, r0, lsr r2
    and     r1, r1, #7
    cmp     r1, #2
    blt     skip
    mcr     p15, #2, r10, c0, c0, #0
    isb
    mrc     p15, #1, r1, c0, c0, #0
    and     r2, r1, #7
    add     r2, r2, #4
    ldr     r4, _FLD_MAX_WAY
    ands    r4, r4, r1, lsr #3
    clz     r5, r4
    ldr     r7, _FLD_MAX_IDX
    ands    r7, r7, r1, lsr #13
loop2:
    mov     r9, r4
loop3:
    orr     r11, r10, r9, lsl r5
    orr     r11, r11, r7, lsl r2
    mcr     p15, #0, r11, c7, c14, #2
    subs    r9, r9, #1
    bge     loop3
    subs    r7, r7, #1
    bge     loop2
skip:
    add     r10, r10, #2
    cmp     r3, r10
    bgt     loop1

finished:
    dsb
    isb
    pop     {r4-r11}
    bx      lr

.globl xport_cpu_icache_flush
xport_cpu_icache_flush:
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 0       @ I+BTB cache invalidate
    dsb
    isb
    bx      lr

.globl xport_cpu_dcache_disable
xport_cpu_dcache_disable:
    push    {r4-r11, lr}
    @bl      xport_cpu_dcache_clean_flush
    mrc     p15, #0, r0, c1, c0, #0
    bic     r0,  r0, #0x00000004
    mcr     p15, #0, r0, c1, c0, #0
    pop     {r4-r11, lr}
    bx      lr

.globl xport_cpu_icache_disable
xport_cpu_icache_disable:
    mrc     p15, #0, r0, c1, c0, #0
    bic     r0,  r0, #0x00001000
    mcr     p15, #0, r0, c1, c0, #0
    bx      lr

.globl xport_cpu_mmu_disable
xport_cpu_mmu_disable:
    mcr     p15, #0, r0, c8, c7, #0    @ invalidate tlb
    mrc     p15, #0, r0, c1, c0, #0
    bic     r0, r0, #1
    mcr     p15, #0, r0, c1, c0, #0    @ clear mmu bit
    dsb
    bx      lr

.globl xport_cpu_mmu_enable
xport_cpu_mmu_enable:
    mrc     p15, #0, r0, c1, c0, #0
    orr     r0, r0, #0x001
    mcr     p15, #0, r0, c1, c0, #0    @ set mmu enable bit
    dsb
    bx      lr

.globl xport_cpu_tlb_set
xport_cpu_tlb_set:
    mcr     p15, #0, r0, c2, c0, #0
    isb     sy
    mcr     p15, #0, r1, c2, c0, #1
    isb     sy
    dmb
    bx      lr

.global xport_cpu_disable_cache
xport_cpu_disable_cache:
    mrc     p15, #0, r0, c1, c0, #0
    bic     r0,  r0, #(1 << 2)
    bic     r0,  r0, #(1 << 12)
    mcr     p15, #0, r0, c1, c0, #0
    bx      lr

@******************************************************************************
@ Functions:
@     void xport_dcache_clean_all(void);
@ reference:
@   "ARM Architecture Reference Manual ARMv7-A and ARMv7-R edition"
@   --Example code for cache maintenance operations
@******************************************************************************
.global xport_dcache_clean_all
xport_dcache_clean_all:
    PUSH {R4-R11}
    MRC p15, #1, R0, c0, c0, #1   @ Read CLIDR into R0
    ANDS R3, R0, #0x07000000
    MOV R3, R3, LSR #23           @ Cache level value (naturally aligned)
    BEQ A_Finished
    MOV R10, #0
A_Loop1:
    ADD R2, R10, R10, LSR #1      @ Work out 3 x cachelevel
    MOV R1, R0, LSR R2            @ bottom 3 bits are the Cache type for this level
    AND R1, R1, #7                @ get those 3 bits alone
    CMP R1, #2
    BLT A_Skip                    @ no cache or only instruction cache at this level
    MCR p15, #2, R10, c0, c0, #0  @ write CSSELR from R10
    ISB                           @ ISB to sync the change to the CCSIDR
    MRC p15, #1, R1, c0, c0, #0   @ read current CCSIDR to R1
    AND R2, R1, #7                @ extract the line length field
    ADD R2, R2, #4                @ add 4 for the line length offset (log2 16 bytes)
    LDR R4, =0x3FF
    ANDS R4, R4, R1, LSR #3       @ R4 is the max number on the way size (right aligned)
    CLZ R5, R4                    @ R5 is the bit position of the way size increment
    MOV R9, R4                    @ R9 working copy of the max way size (right aligned)
A_Loop2:
    LDR R7, =0x00007FFF
    ANDS R7, R7, R1, LSR #13      @ R7 is the max number of the index size (right aligned)
A_Loop3:
    ORR R11, R10, R9, LSL R5      @ factor in the way number and cache number into R11
    ORR R11, R11, R7, LSL R2      @ factor in the index number
    MCR p15, #0, R11, c7, c10, #2 @ DCCSW, Clean data or unified cache line by set/way
    SUBS R7, R7, #1               @ decrement the index
    BGE A_Loop3
    SUBS R9, R9, #1               @ decrement the way number
    BGE A_Loop2
A_Skip:
    ADD R10, R10, #2              @ increment the cache number
    CMP R3, R10
    BGT A_Loop1
    DSB
A_Finished:
    POP {R4-R11}
    BX  LR
