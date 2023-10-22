@ ------------------------------------------------------------
@ armv7.s - common operations
@ date:    2012-2-13 8:42:56
@ author:  Aaron<leafy.myeh@allwinnertech.com>
@ history: V0.1
@
@ Cortex-A7 Basic Operation
@ Operations for Cache and TLB management
@ Operations for VFP/NEON management
@ ------------------------------------------------------------

#include <cpu_asm.h>

	@.section .rodata
	@.align 8
	@.code 32
@ ------------------------------------------------------------

	.extern Image$$TTB$$ZI$$Base    @ from scatter file, translate table address

@------------------------------------------------------------------
	@ Define macro to get dcache line size
	@ In Cortex-A8 dcache line size is 64 bytes
	@ In Cortex-A7 dcache line size is 64 bytes
	.macro DCACHE_LINE_SIZE  REG, TMP
	@DCACHE_LINE_SIZE  $REG, $TMP
	MOV		\REG, #0x40
	.endm
@------------------------------------------------------------------

.global arm_branch_prediction_enable
arm_branch_prediction_enable:
   MRC     p15, 0, r0, c1, c0, 0                                @ Read SCTLR
   ORR     r0, r0, #0x0800                                      @ Set the Z bit in CTRL_C1
   MCR     p15, 0,r0, c1, c0, 0                                 @ Write SCTLR
   DSB
   BX      lr

.global disable_strict_align_check
disable_strict_align_check:
   PUSH		{r0, lr}
   MRC 		p15, 0, r0, c1, c0, 0
   BIC		r0, r0, #(0x1<<1)
   POP		{r0, pc}
   BX		lr
@------------------------------------------------------------------

.global ARMV7_PREDICTION_ENABLE
	@ void ARMV7_PREDICTION_ENABLE(void)
	@ prediction enable and initial the return stack for simulation
ARMV7_PREDICTION_ENABLE:
	PUSH	{lr}
	BL 		rd_label0                  @ Initial Return Stack to prevent 'x' for simulation
rd_label0:
	BL 		rd_label1
rd_label1:
	BL 		rd_label2
rd_label2:
	BL 		rd_label3
rd_label3:
	BL 		rd_label4
rd_label4:
	BL 		rd_label5
rd_label5:
	BL 		rd_label6
rd_label6:
	BL 		rd_label7
rd_label7:

	MOV		r0, #0
	MCR     p15, 0, r0, c7, c5, 6       @ invalid BTAC
	ISB

	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 register 1
	ORR     r0, r0, #(0x1  <<11)        @ Enable all forms of branch prediction
	MCR     p15, 0, r0, c1, c0, 0       @ write CP15 register 1

	POP		{pc}
@------------------------------------------------------------------

@------------------------------------------------------------------
.global ARMV7_L1_ICACHE_ENABLE
	@ void ARMV7_L1_ICACHE_ENABLE(void)
	@ L1 I-Cache enable

ARMV7_L1_ICACHE_ENABLE:

	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 register 1
	ORR     r0, r0, #(0x1  <<12)        @ enable I Cache
	MCR     p15, 0, r0, c1, c0, 0       @ write CP15 register 1

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global ARMV7_L1_DCACHE_ENABLE
	@ void ARMV7_L1_DCACHE_ENABLE(void)
	@ L1 D-Cache enable for Cortex-A8
	@ L1 and L2 D-Cache enable for Cortex-A7

ARMV7_L1_DCACHE_ENABLE:
	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 register 1
	ORR     r0, r0, #(0x1  <<2)         @ enable D Cache
	MCR     p15, 0, r0, c1, c0, 0       @ write CP15 register 1

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global ARMV7_L2_CACHE_ENABLE
	@ void ARMV7_L2_CACHE_ENABLE(void)
	@ L2 Cache enable
	@ Does not apply to Cortex-A7, on Cortex-A7 platform L1 and L2 are
	@ simultaneously enabled by setting P15.SCTRL.C bit

ARMV7_L2_CACHE_ENABLE:
	MRC     p15, 0, r0, c1, c0, 1       @ Read Auxiliary Control Register
	ORR     r0, #2                      @ L2EN bit, enable L2 cache
	MCR     p15, 0, r0, c1, c0, 1       @ Write Auxiliary Control Register

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global ARMV7_MMU_ENABLE
	@ void ARMV7_MMU_ENABLE(void)
	@ MMU enable

ARMV7_MMU_ENABLE:

	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 register 1
	ORR     r0, r0, #0x1                @ enable MMU before scatter loading
	MCR     p15, 0, r0, c1, c0, 0       @ write CP15 register 1

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global ARMV7_ALL_CACHE_DISABLE
	@ void ARMV7_ALL_CACHE_DISABLE(void)
	@ Disable All Cache and MMU

ARMV7_ALL_CACHE_DISABLE:

	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 System Control register
	BIC     r0, r0, #(0x1 << 12)        @ Clear I bit 12 to disable I Cache
	BIC     r0, r0, #(0x1 <<  2)        @ Clear C bit  2 to disable D Cache
	BIC     r0, r0, #0x1                @ Clear M bit  0 to disable MMU
	BIC     r0, r0, #(0x1 << 11)        @ Clear Z bit 11 to disable branch prediction
	MCR     p15, 0, r0, c1, c0, 0       @ Write value back to CP15 System Control register
	DSB

	BX      lr
@------------------------------------------------------------------

@==================================================================
@------------------------------------------------------------------
.global ARMV7_FLUSH_DCACHE_ALL
	@ void ARMV7_FLUSH_DCACHE_ALL(void)
	@ Flush DCache all

.global ARMV7_FLUSH_DCACHE_ALL
ARMV7_FLUSH_DCACHE_ALL:
	PUSH	{r4-r12, lr}

	MRC     p15, 1, r0, c0, c0, 1       @ read clidr
	ANDS    r3, r0, #0x7000000          @ extract loc from clidr
	MOV     r3, r3, lsr #23             @ left align loc bit field
	BEQ     finished                    @ if loc is 0, then no need to clean

	mov     r10, #0                     @ start clean at cache level 0
loop1:
	ADD     r2, r10, r10, lsr #1        @ work out 3x current cache level
	MOV     r1, r0, lsr r2              @ extract cache type bits from clidr
	AND     r1, r1, #7                  @ mask of the bits for current cache only
	CMP     r1, #2                      @ see what cache we have at this level
	BLT     skip                        @ skip if no cache, or just i-cache
	MCR     p15, 2, r10, c0, c0, 0      @ select current cache level in cssr
	ISB                                 @ isb to sych the new cssr&csidr
	MRC     p15, 1, r1, c0, c0, 0       @ read the new csidr
	AND     r2, r1, #7                  @ extract the length of the cache lines
	ADD     r2, r2, #4                  @ add 4 (line length offset)
	LDR     r4, =0x3ff
	ANDS    r4, r4, r1, lsr #3          @ find maximum number on the way size
	CLZ     r5, r4                      @ find bit position of way size increment
	LDR     r7, =0x7fff
	ANDS    r7, r7, r1, lsr #13         @ extract max number of the index size
loop2:
    MOV     r9, r4                      @ create working copy of max way size
loop3:
	ORR     r11, r10, r9, lsl r5        @ factor way and cache number into r11
	ORR     r11, r11, r7, lsl r2        @ factor index number into r11
	MCR     p15, 0, r11, c7, c14, 2     @ clean & invalidate by set/way
	SUBS    r9, r9, #1                  @ decrement the way
	BGE     loop3
	SUBS    r7, r7, #1                  @ decrement the index
	BGE     loop2
skip:
	ADD     r10, r10, #2                @ increment cache number
	CMP     r3, r10
	BGT     loop1
finished:
	MOV     r10, #0                     @ swith back to cache level 0
	MCR     p15, 2, r10, c0, c0, 0      @ select current cache level in cssr
	ISB

	POP	{r4-r12, pc}
@------------------------------------------------------------------

@------------------------------------------------------------------
.global ARMV7_FLUSH_ICACHE_ALL
	@ void ARMV7_FLUSH_ICACHE_ALL(void)
	@ Flush ICache all

ARMV7_FLUSH_ICACHE_ALL:

	MRC     p15, 1, r0, c0, c0, 1      @ Read Cache Level ID Register (CLIDR)
	TST     r0, #0x3                   @ Harvard Cache?
	MOV     r0, #0                     @ SBZ
	MCRNE   p15, 0, r0, c7, c5, 0      @ ICIALLU - Invalidate instruction cache and flush branch target cache

	BX      lr
@------------------------------------------------------------------


@------------------------------------------------------------------
.global ARMV7_FLUSH_CACHE_ALL
	@ void ARMV7_FLUSH_CACHE_ALL(void)
	@ Cache Invalidation code for Cortex-A7
	@ NOTE: Neither Caches, nor MMU, nor BTB need post-reset invalidation on Cortex-A7,
	@ but forcing a cache invalidation, makes the code more portable to other CPUs (e.g. Cortex-A9)

ARMV7_FLUSH_CACHE_ALL:

	PUSH	{lr}

	BL      ARMV7_FLUSH_ICACHE_ALL      @ flush icache all
	BL      ARMV7_FLUSH_DCACHE_ALL      @ flush dcache all

	POP     {pc}
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_FLUSH_ICACHE_REGION
	@ void ARMV7_FLUSH_ICACHE_REGION(void *start, void *end)@
	@ Flush ICache Region from start to end

ARMV7_FLUSH_ICACHE_REGION:

	DCACHE_LINE_SIZE r2, r3
	SUB     r3, r2, #1
	BIC     r0, r0, r3
1:
	MCR     p15, 0, r0, c7, c11, 1      @ clean D line to the point of unification
	DSB
	MCR     p15, 0, r0, c7, c5, 1       @ invalidate I line
	ADD     r0, r0, r2
	CMP     r0, r1
	BLO     1b
	MOV     r0, #0
	MCR     p15, 0, r0, c7, c5, 6       @ invalidate BTB
	DSB
	ISB

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_FLUSH_DCACHE_REGION
	@ void ARMV7_FLUSH_DCACHE_REGION(void *start, void *end)@
	@ Flush DCache Region from start to end

ARMV7_FLUSH_DCACHE_REGION:

	DCACHE_LINE_SIZE r2, r3
	SUB     r3, r2, #1
	BIC     r0, r0, r3
1:
	MCR     p15, 0, r0, c7, c14, 1      @ clean & invalidate D / U line
	ADD     r0, r0, r2
	CMP     r0, r1
	BLO     1b
	DSB

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_FLUSH_DCACHE_REGION
	@ void ARMV7_CLEAN_DCACHE_REGION(void *start, void *end)@
	@ Clean DCache Region from start to end

ARMV7_CLEAN_DCACHE_REGION:

	DCACHE_LINE_SIZE r2, r3
	SUB     r3, r2, #1
	BIC     r0, r0, r3
1:
	MCR     p15, 0, r0, c7, c10, 1      @ clean D / U line
	ADD     r0, r0, r2
	CMP     r0, r1
	BLO     1b
	DSB

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_INVALID_DCACHE_REGION
	@ void ARMV7_INVALID_DCACHE_REGION(void *start, void *end)@
	@ Invalid DCache Region from start to end

ARMV7_INVALID_DCACHE_REGION:

	DCACHE_LINE_SIZE r2, r3
	SUB     r3, r2, #1
	TST     r0, r3
	BIC     r0, r0, r3
	MCRNE   p15, 0, r0, c7, c14, 1      @ clean & invalidate D / U line

	TST     r1, r3
	BIC     r1, r1, r3
	MCRNE   p15, 0, r1, c7, c14, 1      @ clean & invalidate D / U line
1:
	MCR     p15, 0, r0, c7, c6, 1       @ invalidate D / U line
	ADD     r0, r0, r2
	CMP     r0, r1
	BLO     1b
	DSB

	BX      lr
@------------------------------------------------------------------


@------------------------------------------------------------------
.global	ARMV7_FLUSH_TLB_ALL
	@ void ARMV7_FLUSH_TLB_ALL(void)@
	@ Invalid TLB

ARMV7_FLUSH_TLB_ALL:

	DSB
	MOV     r0, #0
	MCR     p15, 0, r0, c8, c5, 0    @ I-TLB invalidation
	MCR     p15, 0, r0, c8, c6, 0    @ D-TLB invalidation
	MCR     p15, 0, r0, c8, c7, 0    @ Unified-TLB invalidation
	DSB
	ISB

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_MMU_TABLE_INIT
	@ void ARMV7_MMU_TABLE_INIT(void)

	@ PAGE TABLE generation
	@ Generate the page tables
	@ Build a flat translation table for the whole address space.
	@
	@ 31                 20 19  18  17  16 15  14   12 11 10  9  8     5   4    3 2   1 0
	@ |section base address| 0  0  |nG| S |AP2|  TEX  |  AP | P | Domain | XN | C B | 1 0|
	@
	@ Bits[31:20]   - Top 12 bits of VA is pointer into table
	@ nG[17]=0      - Non global, enables matching against ASID in the TLB when set.
	@ S[16]=0       - Indicates normal memory is shared when set.
	@ AP2[15]=0
	@ AP[11:10]=11  - Configure for full read/write access in all modes
	@ TEX[14:12]=000
	@ CB[3:2]= 00   - Set attributes to Strongly-ordered memory.
	@                 (except for the 0x600 descriptor, see below)
	@ IMPP[9]=0     - Ignored
	@ Domain[5:8]=1111   - Set all pages to use domain 15
	@ XN[4]=0       - Execute never disabled
	@ Bits[1:0]=10  - Indicate entry is a 1MB section
	@
	@ TEX  C  B	Description                                             Memory Type
	@ 000  0  0	Strongly-Ordered,                                       Strongly-Ordered    Shareable
	@      0  1	Shareable Device,                                       Device              Shareable
	@      1  0	Outer and Inner Write-Through, no Write-allocate        Normal              Outer Shareable
	@      1  1	Outer and Inner Write-Back, no Write-allocate           Normal              Outer Shareable
	@
	@ 001  0  0	Outer and inner Non-cacheable                           Normal              Outer Shareable
	@      0  1	reserved
	@      1  0	Outer and inner write back, write allocate              Normal              Outer Shareable
	@      1  1	reserved
	@
	@ 010  0  0	Non-shareable Device                                    Device              Non-Shareable
	@      0  1	reserved                                                -
	@      1  x	reserved                                                -
	@
	@ 011  x  x	reserved

	@ write the address of our page table base to TTB register 0
	@ A7-CORE0 TTB BASE - 0x20000			 core0123 share this TLB (used in code)
	@ A7-CORE1 TTB BASE - 0x20000 + 0x4000
	@ A7-CORE2 TTB BASE - 0x20000 + 0x8000
	@ A7-CORE3 TTB BASE - 0x20000 + 0xc000

ARMV7_MMU_TABLE_INIT:

	@LDR     r0,=Image$$TTB$$ZI$$Base            @ Get TTB Base
	LDR     r0,=__tlb_base_entry          @ Get TTB Base
	LDR     r1,=0xfff                               @ loop counter
	@LDR	r2,=0b00000000000000000000110111101110  @ Inner & Outer Write Back
	@LDR	r2,=0b00000000000000000000110111101010  @ Inner & Outer Write Through
	LDR     r2,=0b00000000000000000001110111100010  @ Normal Non-Cacheable for Memory

	@ r0 contains the address of the translation table base
	@ r1 is loop counter
	@ r2 is level1 descriptor (bits 19:0)

	@ use loop counter to create 4096 individual table entries.
	@ this writes from address 'Image$$TTB$$ZI$$Base' +
	@ offset 0x3FFC down to offset 0x0 in word steps (4 bytes)

init_ttb_1:
	@ config tlb discriptor for memory area
	ORR     r3, r2, r1, LSL#20          @ r3 now contains full level1 descriptor to write
	STR     r3, [r0, r1, LSL#2]         @ str table entry at TTB base + loopcount*4
	SUB     r1, r1, #1                  @ decrement loop counter
	CMP     r1, #0x20                   @ check if reach the IO area (<0x02000000)
	BPL     init_ttb_1

	LDR     r2,=0b00000000000000000000110111100010  @ Strongly Order for I/O-Area

init_ttb_2:
	@ config tlb discriptor for io area
	ORR     r3, r2, r1, LSL#20          @ r3 now contains full level1 descriptor to write
	STR     r3, [r0, r1, LSL#2]         @ str table entry at TTB base + loopcount*4
	SUBS    r1, r1, #1                  @ decrement loop counter
	BPL     init_ttb_2

	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_GET_MMU_TABLE_BASE
	@ int ARMV7_GET_MMU_TABLE_BASE(void)@
	@ Get MMU Table Base

ARMV7_GET_MMU_TABLE_BASE:

	@LDR     r0,=Image$$TTB$$ZI$$Base
	LDR     r0,=__tlb_base_entry
	BX      lr
@------------------------------------------------------------------

@------------------------------------------------------------------
.global	ARMV7_NEON_VFP_ENABLE
	@ void ARMV7_NEON_VFP_ENABLE(void)@
	@ Enable NEON/VFP

ARMV7_NEON_VFP_ENABLE:

	@ Enable access to NEON/VFP by enabling access to Coprocessors 10 and 11.
	@ Enables Full Access i.e. in both priv and non priv modes
	MRC     p15, 0, r0, c1, c0, 2      @ Read Coprocessor Access Control Register (CPACR)
	ORR     r0, r0, #(0xF << 20)       @ Enable access to CP 10 & 11
	MCR     p15, 0, r0, c1, c0, 2      @ Write Coprocessor Access Control Register (CPACR)
	ISB

	@ Switch on the VFP and Neon Hardware
	MOV     r0, #0x40000000
	VMSR    FPEXC, r0					@ Write FPEXC register, EN bit set

	BX      lr

@-------------------------------------------------------------------
.global	ARMV7_IRQ_ENABLE
	@ void ARMV7_IRQ_ENABLE(void)@
	@ Enable IRQ of CPU

ARMV7_IRQ_ENABLE:

	MRS     r0, cpsr
	@BIC     r0, r0, #ARMV7_IRQ_MASK
	BIC     r0, r0, #(1<<7)
	MSR     cpsr_c, r0

	BX      lr
@-------------------------------------------------------------------

@-------------------------------------------------------------------
.global	ARMV7_IRQ_DISABLE
	@ void ARMV7_IRQ_DISABLE(void)@
	@ Disable IRQ of CPU

ARMV7_IRQ_DISABLE:
	MRS     r0, cpsr
	@ORR     r0, r0, #ARMV7_IRQ_MASK
	ORR     r0, r0, #(1<<7)
	MSR     cpsr_c, r0

	BX      lr
@-------------------------------------------------------------------

@-------------------------------------------------------------------
.global	ARMV7_FIQ_ENABLE
	@ void ARMV7_FIQ_ENABLE(void)@
	@ Disable FIQ of CPU

ARMV7_FIQ_ENABLE:

	MRS     r0, cpsr
	@BIC     r0, r0, #ARMV7_FIQ_MASK
	BIC     r0, r0, #(1<<6)
	MSR     cpsr_c, r0

	BX      lr
@-------------------------------------------------------------------

@-------------------------------------------------------------------
.global	ARMV7_FIQ_DISABLE
	@ void ARMV7_FIQ_DISABLE(void)@
	@ Disable FIQ of CPU

ARMV7_FIQ_DISABLE:

	MRS     r0, cpsr
	ORR     r0, r0, #(1<<6)
	MSR     cpsr_c, r0

	BX      lr
@-------------------------------------------------------------------

@-------------------------------------------------------------------
.global	ARMV7_VECT_HIGH_ENABLE
.global	ARMV7_VECT_HIGH_DISABLE
	@ void ARMV7_VECT_HIGH_ENABLE(void)@
	@ void ARMV7_VECT_HIGH_DISABLE(void)@
	@ Vector High (0xffff0000) or Vector Low (0x00)

ARMV7_VECT_HIGH_ENABLE:

	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 register 1
	ORR     r0, r0, #(0x1  <<13)        @ enable vect high (0xffff0000)
	MCR     p15, 0, r0, c1, c0, 0       @ write CP15 register 1

	BX      lr

ARMV7_VECT_HIGH_DISABLE:

	MRC     p15, 0, r0, c1, c0, 0       @ read CP15 register 1
	BIC     r0, r0, #(0x1  <<13)        @ disable vect high (0xffff0000)
	MCR     p15, 0, r0, c1, c0, 0       @ write CP15 register 1

	BX      lr

@-------------------------------------------------------------------
.global ARMV7_INVALIDATE_DCACHE
	@void ARMV7_INVALIDATE_DCACHE(void)
ARMV7_INVALIDATE_DCACHE:
	PUSH    {r0-r12}

	@Based on code example given in section 11.2.4 of ARM DDI 0406B
	MRC     p15, 1, r0, c0, c0, 1     @ Read CLIDR
	ANDS    r3, r0, #0x7000000
	MOV     r3, r3, LSR #23           @ Cache level value (naturally aligned)
	BEQ	CLEAN_INVALIDATE_DCACHE_FINISHED
	MOV     r10, #0

CLEAN_INVALIDATE_DCACHE_LOOP1:
	ADD     r2, r10, r10, LSR #1      @ Work out 3xcachelevel
	MOV     r1, r0, LSR r2            @ bottom 3 bits are the Cache type for this level
	AND     r1, r1, #7                @ get those 3 bits alone
	CMP     r1, #2
	BLT     CLEAN_INVALIDATE_DCACHE_SKIP @  no cache or only instruction cache at this level
	MCR     p15, 2, r10, c0, c0, 0    @  write the Cache Size selection register
	ISB                               @  ISB to sync the change to the CacheSizeID reg
	MRC     p15, 1, r1, c0, c0, 0     @  reads current Cache Size ID register
	AND     r2, r1, #0x7              @  extract the line length field
	ADD     r2, r2, #4                @  add 4 for the line length offset (log2 16 bytes)
	LDR     r4, =0x3FF
	ANDS    r4, r4, r1, LSR #3        @  R4 is the max number on the way size (right aligned)
	CLZ     r5, r4                    @  R5 is the bit position of the way size increment
	LDR     r7, =0x00007FFF
	ANDS    r7, r7, r1, LSR #13       @  R7 is the max number of the index size (right aligned)

CLEAN_INVALIDATE_DCACHE_LOOP2:
	MOV     r9, R4                    @  R9 working copy of the max way size (right aligned)

CLEAN_INVALIDATE_DCACHE_LOOP3:
	ORR     r11, r10, r9, LSL r5      @  factor in the way number and cache number into R11
	ORR     r11, r11, r7, LSL r2      @  factor in the index number
	@MCR     p15, 0, r11, c7, c14, 2  @  DCCISW - clean and invalidate by set/way
	MCR     p15, 0, r11, c7, c6, 2    @  DCISW - Invalidate by set/way
	SUBS    r9, r9, #1                @  decrement the way number
	BGE     CLEAN_INVALIDATE_DCACHE_LOOP3
	SUBS    r7, r7, #1                @  decrement the index
	BGE     CLEAN_INVALIDATE_DCACHE_LOOP2

CLEAN_INVALIDATE_DCACHE_SKIP:
	ADD     r10, r10, #2              @  increment the cache number
	CMP     r3, r10
	BGT     CLEAN_INVALIDATE_DCACHE_LOOP1

CLEAN_INVALIDATE_DCACHE_FINISHED:
	mov     r10, #0                         @ swith back to cache level 0
	mcr     p15, 2, r10, c0, c0, 0          @ select current cache level in cssr
	dsb
	isb
	POP     {r0-r12}

	BX      lr

@-------------------------------------------------------------------

.global v7_dma_map_area
v7_dma_map_area:
    add     r1, r1, r0
    teq     r2, #DMA_DEV_TO_MEM
    beq     ARMV7_INVALID_DCACHE_REGION
    teq     r2, #DMA_MEM_TO_MEM
    beq     ARMV7_INVALID_DCACHE_REGION
    b       ARMV7_CLEAN_DCACHE_REGION

.global v7_dma_unmap_area
v7_dma_unmap_area:
    add     r1, r1, r0
    teq     r2, #DMA_DEV_TO_MEM
    beq     ARMV7_INVALID_DCACHE_REGION
    teq     r2, #DMA_MEM_TO_MEM
    beq     ARMV7_INVALID_DCACHE_REGION
    bx      lr

.end
