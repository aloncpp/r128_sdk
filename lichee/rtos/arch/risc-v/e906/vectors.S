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
/* Enable interrupts when returning from the handler */
#define MSTATUS_PRV1 0x1880

#define store_x sw
#define load_x lw
#define portWORD_SIZE 4
#ifdef CONFIG_ARCH_RISCV_FPU
#define portCONTEXT_SIZE ( 68 * portWORD_SIZE )
#define FSTORE                  fsw
#define FLOAD                   flw
#define FREGBYTES               4
#else
#define portCONTEXT_SIZE ( 34 * portWORD_SIZE )
#endif

.section .bss
    .align  2
    .globl  g_trapstackalloc
    .global g_trapstackbase
    .global g_top_trapstack
g_trapstackalloc:
g_trapstackbase:
    .space 768
g_top_trapstack:

    .align 2
    .globl g_trap_sp
    .type  g_trap_sp, object
g_trap_sp:
    .long 0
    .size g_trap_sp, .-g_trap_sp


irq_nested_level:
.long 0

.text

    .align  6
    .global Default_IRQHandler
    .weak   Default_IRQHandler
    .type   Default_IRQHandler, %function
Default_IRQHandler:
	addi sp, sp, -portCONTEXT_SIZE
	store_x x1,  1  * portWORD_SIZE( sp )
	store_x x5,  2  * portWORD_SIZE( sp )
	store_x x6,  3  * portWORD_SIZE( sp )
	store_x x7,  4  * portWORD_SIZE( sp )
	store_x x8,  5  * portWORD_SIZE( sp )
	store_x x9,  6  * portWORD_SIZE( sp )
	store_x x10, 7  * portWORD_SIZE( sp )
	store_x x11, 8  * portWORD_SIZE( sp )
	store_x x12, 9  * portWORD_SIZE( sp )
	store_x x13, 10 * portWORD_SIZE( sp )
	store_x x14, 11 * portWORD_SIZE( sp )
	store_x x15, 12 * portWORD_SIZE( sp )
	store_x x16, 13 * portWORD_SIZE( sp )
	store_x x17, 14 * portWORD_SIZE( sp )
	store_x x18, 15 * portWORD_SIZE( sp )
	store_x x19, 16 * portWORD_SIZE( sp )
	store_x x20, 17 * portWORD_SIZE( sp )
	store_x x21, 18 * portWORD_SIZE( sp )
	store_x x22, 19 * portWORD_SIZE( sp )
	store_x x23, 20 * portWORD_SIZE( sp )
	store_x x24, 21 * portWORD_SIZE( sp )
	store_x x25, 22 * portWORD_SIZE( sp )
	store_x x26, 23 * portWORD_SIZE( sp )
	store_x x27, 24 * portWORD_SIZE( sp )
	store_x x28, 25 * portWORD_SIZE( sp )
	store_x x29, 26 * portWORD_SIZE( sp )
	store_x x30, 27 * portWORD_SIZE( sp )
	store_x x31, 28 * portWORD_SIZE( sp )
	store_x x3,  31 * portWORD_SIZE( sp )
	store_x x4,  32 * portWORD_SIZE( sp )
	csrr a0, mepc
	store_x a0,  0 * portWORD_SIZE( sp )
	csrr t0, mscratch
	store_x t0, 33 * portWORD_SIZE( sp )

	csrr t0, mstatus
	store_x t0, 29 * portWORD_SIZE( sp )

#ifdef CONFIG_ARCH_RISCV_FPU
	/* mstatus.FS[14:13]
	 * 2'b00: FPU Disable
	 * 2'b01: FPU Init status
	 * 2'b10: FPU Register is clean, not need to save
	 * 2'b11: FPU Register is dirty, need to save
	*/
    csrr    t1, mstatus
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    li      t0, 0x3
    bne     t1, t0,  .F_RegNotSave1

    addi    t0, sp, (34 * FREGBYTES)
    FSTORE  f0,  0  * FREGBYTES(t0)
    FSTORE  f1,  1  * FREGBYTES(t0)
    FSTORE  f2,  2  * FREGBYTES(t0)
    FSTORE  f3,  3  * FREGBYTES(t0)
    FSTORE  f4,  4  * FREGBYTES(t0)
    FSTORE  f5,  5  * FREGBYTES(t0)
    FSTORE  f6,  6  * FREGBYTES(t0)
    FSTORE  f7,  7  * FREGBYTES(t0)
    FSTORE  f8,  8  * FREGBYTES(t0)
    FSTORE  f9,  9  * FREGBYTES(t0)
    FSTORE  f10, 10 * FREGBYTES(t0)
    FSTORE  f11, 11 * FREGBYTES(t0)
    FSTORE  f12, 12 * FREGBYTES(t0)
    FSTORE  f13, 13 * FREGBYTES(t0)
    FSTORE  f14, 14 * FREGBYTES(t0)
    FSTORE  f15, 15 * FREGBYTES(t0)
    FSTORE  f16, 16 * FREGBYTES(t0)
    FSTORE  f17, 17 * FREGBYTES(t0)
    FSTORE  f18, 18 * FREGBYTES(t0)
    FSTORE  f19, 19 * FREGBYTES(t0)
    FSTORE  f20, 20 * FREGBYTES(t0)
    FSTORE  f21, 21 * FREGBYTES(t0)
    FSTORE  f22, 22 * FREGBYTES(t0)
    FSTORE  f23, 23 * FREGBYTES(t0)
    FSTORE  f24, 24 * FREGBYTES(t0)
    FSTORE  f25, 25 * FREGBYTES(t0)
    FSTORE  f26, 26 * FREGBYTES(t0)
    FSTORE  f27, 27 * FREGBYTES(t0)
    FSTORE  f28, 28 * FREGBYTES(t0)
    FSTORE  f29, 29 * FREGBYTES(t0)
    FSTORE  f30, 30 * FREGBYTES(t0)
    FSTORE  f31, 31 * FREGBYTES(t0)
    frcsr    t1
    store_x t1,  32 * FREGBYTES(t0)

.F_RegNotSave1:
#endif

    lw      t0, pxCurrentTCB		/* Load pxCurrentTCB. */
    sw      sp, 0( t0 )				/* Write sp to first TCB member. */

    csrr    t1, mcause
    andi    t1, t1, 0x3FF
    slli    t1, t1, 2
    la      t0, g_irqvector
    add     t0, t0, t1
    lw      t2, (t0)
    jalr    t2

    li      t0, MSTATUS_PRV1
    csrs    mstatus, t0

    call    vTaskSwitchContext

    lw      t1, pxCurrentTCB			/* Load pxCurrentTCB. */
    lw      sp, 0( t1 )				 	/* Read sp from first TCB member. */

#ifdef CONFIG_ARCH_RISCV_FPU
    load_x  t1, 29 * portWORD_SIZE( sp )
    csrw    mstatus, t1
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    li      t0, 0x3
    bne  t0, t1, .F_RegNotLoad

    addi    t1, sp, (34 * FREGBYTES)
    FLOAD f0, 0  * FREGBYTES(t1)
    FLOAD f1, 1  * FREGBYTES(t1)
    FLOAD f2, 2  * FREGBYTES(t1)
    FLOAD f3, 3  * FREGBYTES(t1)
    FLOAD f4, 4  * FREGBYTES(t1)
    FLOAD f5, 5  * FREGBYTES(t1)
    FLOAD f6, 6  * FREGBYTES(t1)
    FLOAD f7, 7  * FREGBYTES(t1)
    FLOAD f8, 8  * FREGBYTES(t1)
    FLOAD f9, 9  * FREGBYTES(t1)
    FLOAD f10,10 * FREGBYTES(t1)
    FLOAD f11,11 * FREGBYTES(t1)
    FLOAD f12,12 * FREGBYTES(t1)
    FLOAD f13,13 * FREGBYTES(t1)
    FLOAD f14,14 * FREGBYTES(t1)
    FLOAD f15,15 * FREGBYTES(t1)
    FLOAD f16,16 * FREGBYTES(t1)
    FLOAD f17,17 * FREGBYTES(t1)
    FLOAD f18,18 * FREGBYTES(t1)
    FLOAD f19,19 * FREGBYTES(t1)
    FLOAD f20,20 * FREGBYTES(t1)
    FLOAD f21,21 * FREGBYTES(t1)
    FLOAD f22,22 * FREGBYTES(t1)
    FLOAD f23,23 * FREGBYTES(t1)
    FLOAD f24,24 * FREGBYTES(t1)
    FLOAD f25,25 * FREGBYTES(t1)
    FLOAD f26,26 * FREGBYTES(t1)
    FLOAD f27,27 * FREGBYTES(t1)
    FLOAD f28,28 * FREGBYTES(t1)
    FLOAD f29,29 * FREGBYTES(t1)
    FLOAD f30,30 * FREGBYTES(t1)
    FLOAD f31,31 * FREGBYTES(t1)
    load_x t0, 32 * 4(t1)
    fscsr t0
    # clr FS domain
    li  t0, 0x6000
    csrc mstatus, t0
    #clean status would clr sr_sd;
    li  t0, 0x4000
    csrs mstatus, t0
    # save new mstatus
    csrr t0, mstatus
    store_x t0, 29 * portWORD_SIZE( sp )
.F_RegNotLoad:
#endif
	load_x  t0, 0  * portWORD_SIZE( sp )
	csrw mepc, t0

	/* Load mstatus with the interrupt enable bits used by the task. */
	load_x  t0, 29 * portWORD_SIZE( sp )
	csrw mstatus, t0

	load_x  t0, 33 * portWORD_SIZE( sp )
	csrw mscratch, t0

	load_x  x1 , 1  * portWORD_SIZE( sp )
	load_x  x5 , 2  * portWORD_SIZE( sp )		/* t0 */
	load_x  x6 , 3  * portWORD_SIZE( sp )		/* t1 */
	load_x  x7 , 4  * portWORD_SIZE( sp )		/* t2 */
	load_x  x8 , 5  * portWORD_SIZE( sp )		/* s0/fp */
	load_x  x9 , 6  * portWORD_SIZE( sp )		/* s1 */
	load_x  x10, 7  * portWORD_SIZE( sp )	/* a0 */
	load_x  x11, 8  * portWORD_SIZE( sp )	/* a1 */
	load_x  x12, 9  * portWORD_SIZE( sp )	/* a2 */
	load_x  x13, 10 * portWORD_SIZE( sp )	/* a3 */
	load_x  x14, 11 * portWORD_SIZE( sp )	/* a4 */
	load_x  x15, 12 * portWORD_SIZE( sp )	/* a5 */
	load_x  x16, 13 * portWORD_SIZE( sp )	/* a6 */
	load_x  x17, 14 * portWORD_SIZE( sp )	/* a7 */
	load_x  x18, 15 * portWORD_SIZE( sp )	/* s2 */
	load_x  x19, 16 * portWORD_SIZE( sp )	/* s3 */
	load_x  x20, 17 * portWORD_SIZE( sp )	/* s4 */
	load_x  x21, 18 * portWORD_SIZE( sp )	/* s5 */
	load_x  x22, 19 * portWORD_SIZE( sp )	/* s6 */
	load_x  x23, 20 * portWORD_SIZE( sp )	/* s7 */
	load_x  x24, 21 * portWORD_SIZE( sp )	/* s8 */
	load_x  x25, 22 * portWORD_SIZE( sp )	/* s9 */
	load_x  x26, 23 * portWORD_SIZE( sp )	/* s10 */
	load_x  x27, 24 * portWORD_SIZE( sp )	/* s11 */
	load_x  x28, 25 * portWORD_SIZE( sp )	/* t3 */
	load_x  x29, 26 * portWORD_SIZE( sp )	/* t4 */
	load_x  x30, 27 * portWORD_SIZE( sp )	/* t5 */
	load_x  x31, 28 * portWORD_SIZE( sp )	/* t6 */
	addi sp, sp, portCONTEXT_SIZE

	mret

/******************************************************************************
 * Functions:
 *     void trap(void);
 * default exception handler
 ******************************************************************************/
    .align  6
    .global trap
    .type   trap, %function
trap:
    /* Check for interrupt */
    addi    sp, sp, -4
    sw      t0, 0x0(sp)
    csrr    t0, mcause
    blt     t0, x0, .Lirq
    addi    sp, sp, 4
    la      t0, g_trap_sp

    addi    t0, t0, -132

    sw      x1,    0(t0)
    sw      x2,    4(t0)
    sw      x3,    8(t0)
    sw      x4,   12(t0)
    sw      x6,   20(t0)
    sw      x7,   24(t0)
    sw      x8,   28(t0)
    sw      x9,   32(t0)
    sw      x10,  36(t0)
    sw      x11,  40(t0)
    sw      x12,  44(t0)
    sw      x13,  48(t0)
    sw      x14,  52(t0)
    sw      x15,  56(t0)
    sw      x16,  60(t0)
    sw      x17,  64(t0)
    sw      x18,  68(t0)
    sw      x19,  72(t0)
    sw      x20,  76(t0)
    sw      x21,  80(t0)
    sw      x22,  84(t0)
    sw      x23,  88(t0)
    sw      x24,  92(t0)
    sw      x25,  96(t0)
    sw      x26, 100(t0)
    sw      x27, 104(t0)
    sw      x28, 108(t0)
    sw      x29, 112(t0)
    sw      x30, 116(t0)
    sw      x31, 120(t0)

    csrr    a0, mepc
    sw      a0, 124(t0)
    csrr    a0, mstatus
    sw      a0, 128(t0)

    mv      a0, t0
    lw      t0, -4(sp)
    mv      sp, a0
    sw      t0, 16(sp)

    jal     trap_c


.Lirq:
    lw      t0, 0x0(sp)
    addi    sp, sp, 4
    j       Default_IRQHandler

    .align  6
    .weak   Default_Handler
    .global Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    /* Check for nmi */
    addi    sp, sp, -8
    sw      t0, 0x0(sp)
    sw      t1, 0x4(sp)
    csrr    t0, mcause
    andi    t0, t0, 0x3FF
    li      t1, 24
    beq     t0, t1, .NMI_Handler
    lw      t0, 0x0(sp)
    lw      t1, 0x4(sp)
    addi    sp, sp, 8
    j       trap

.NMI_Handler:
    lw      t0, 0x0(sp)
    lw      t1, 0x4(sp)
    addi    sp, sp, 8

    addi    sp, sp, -64

    sw      ra,  0(sp)
    sw      t0,  4(sp)
    sw      t1,  8(sp)
    sw      t2, 12(sp)
    sw      a0, 16(sp)
    sw      a1, 20(sp)
    sw      a2, 24(sp)
    sw      a3, 28(sp)
    sw      a4, 32(sp)
    sw      a5, 36(sp)
    sw      a6, 40(sp)
    sw      a7, 44(sp)
    sw      t3, 48(sp)
    sw      t4, 52(sp)
    sw      t5, 56(sp)
    sw      t6, 60(sp)

#ifdef CONFIG_ARCH_RISCV_FPU
	/* mstatus.FS[14:13]
	 * 2'b00: FPU Disable
	 * 2'b01: FPU Init status
	 * 2'b10: FPU Register is clean, not need to save
	 * 2'b11: FPU Register is dirty, need to save
	*/
    csrr    t1, mstatus
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    li      t0, 0x3
    bne     t1, t0,  .F_RegNotSave2

    addi    t0, sp, (34 * FREGBYTES)
    FSTORE  f0,  0  * FREGBYTES(t0)
    FSTORE  f1,  1  * FREGBYTES(t0)
    FSTORE  f2,  2  * FREGBYTES(t0)
    FSTORE  f3,  3  * FREGBYTES(t0)
    FSTORE  f4,  4  * FREGBYTES(t0)
    FSTORE  f5,  5  * FREGBYTES(t0)
    FSTORE  f6,  6  * FREGBYTES(t0)
    FSTORE  f7,  7  * FREGBYTES(t0)
    FSTORE  f8,  8  * FREGBYTES(t0)
    FSTORE  f9,  9  * FREGBYTES(t0)
    FSTORE  f10, 10 * FREGBYTES(t0)
    FSTORE  f11, 11 * FREGBYTES(t0)
    FSTORE  f12, 12 * FREGBYTES(t0)
    FSTORE  f13, 13 * FREGBYTES(t0)
    FSTORE  f14, 14 * FREGBYTES(t0)
    FSTORE  f15, 15 * FREGBYTES(t0)
    FSTORE  f16, 16 * FREGBYTES(t0)
    FSTORE  f17, 17 * FREGBYTES(t0)
    FSTORE  f18, 18 * FREGBYTES(t0)
    FSTORE  f19, 19 * FREGBYTES(t0)
    FSTORE  f20, 20 * FREGBYTES(t0)
    FSTORE  f21, 21 * FREGBYTES(t0)
    FSTORE  f22, 22 * FREGBYTES(t0)
    FSTORE  f23, 23 * FREGBYTES(t0)
    FSTORE  f24, 24 * FREGBYTES(t0)
    FSTORE  f25, 25 * FREGBYTES(t0)
    FSTORE  f26, 26 * FREGBYTES(t0)
    FSTORE  f27, 27 * FREGBYTES(t0)
    FSTORE  f28, 28 * FREGBYTES(t0)
    FSTORE  f29, 29 * FREGBYTES(t0)
    FSTORE  f30, 30 * FREGBYTES(t0)
    FSTORE  f31, 31 * FREGBYTES(t0)
    frcsr    t1
    store_x t1,  32 * 4(t0)
.F_RegNotSave2:
#endif
    la      t0, g_nmivector
    lw      t0, (t0)
    jalr    t0

#ifdef CONFIG_ARCH_RISCV_FPU
    load_x  t1, 29 * portWORD_SIZE( sp )
    csrw    mstatus, t1
    srli    t1, t1, 13
    andi    t1, t1, 0x3
    li      t0, 0x3
    bne  t0, t1, .F_RegNotLoad2

    addi    t1, sp, (34 * FREGBYTES)
    FLOAD f0, 0  * FREGBYTES(t1)
    FLOAD f1, 1  * FREGBYTES(t1)
    FLOAD f2, 2  * FREGBYTES(t1)
    FLOAD f3, 3  * FREGBYTES(t1)
    FLOAD f4, 4  * FREGBYTES(t1)
    FLOAD f5, 5  * FREGBYTES(t1)
    FLOAD f6, 6  * FREGBYTES(t1)
    FLOAD f7, 7  * FREGBYTES(t1)
    FLOAD f8, 8  * FREGBYTES(t1)
    FLOAD f9, 9  * FREGBYTES(t1)
    FLOAD f10,10 * FREGBYTES(t1)
    FLOAD f11,11 * FREGBYTES(t1)
    FLOAD f12,12 * FREGBYTES(t1)
    FLOAD f13,13 * FREGBYTES(t1)
    FLOAD f14,14 * FREGBYTES(t1)
    FLOAD f15,15 * FREGBYTES(t1)
    FLOAD f16,16 * FREGBYTES(t1)
    FLOAD f17,17 * FREGBYTES(t1)
    FLOAD f18,18 * FREGBYTES(t1)
    FLOAD f19,19 * FREGBYTES(t1)
    FLOAD f20,20 * FREGBYTES(t1)
    FLOAD f21,21 * FREGBYTES(t1)
    FLOAD f22,22 * FREGBYTES(t1)
    FLOAD f23,23 * FREGBYTES(t1)
    FLOAD f24,24 * FREGBYTES(t1)
    FLOAD f25,25 * FREGBYTES(t1)
    FLOAD f26,26 * FREGBYTES(t1)
    FLOAD f27,27 * FREGBYTES(t1)
    FLOAD f28,28 * FREGBYTES(t1)
    FLOAD f29,29 * FREGBYTES(t1)
    FLOAD f30,30 * FREGBYTES(t1)
    FLOAD f31,31 * FREGBYTES(t1)
    load_x t0, 32 * 4(t1)
    fscsr t0
    # clr FS domain
    li  t0, 0x6000
    csrc mstatus, t0
    #clean status would clr sr_sd;
    li  t0, 0x4000
    csrs mstatus, t0
    # save new mstatus
    csrr t0, mstatus
    store_x t0, 29 * portWORD_SIZE( sp )
.F_RegNotLoad2:
#endif
    lw      ra,  0(sp)
    lw      t0,  4(sp)
    lw      t1,  8(sp)
    lw      t2, 12(sp)
    lw      a0, 16(sp)
    lw      a1, 20(sp)
    lw      a2, 24(sp)
    lw      a3, 28(sp)
    lw      a4, 32(sp)
    lw      a5, 36(sp)
    lw      a6, 40(sp)
    lw      a7, 44(sp)
    lw      t3, 48(sp)
    lw      t4, 52(sp)
    lw      t5, 56(sp)
    lw      t6, 60(sp)

    addi    sp, sp, 64
    mret

    .size   Default_Handler, . - Default_Handler

/*    Macro to define default handlers. Default handler
 *    will be weak symbol and just dead loops. They can be
 *    overwritten by other handlers */
    .macro  def_irq_handler handler_name
    .weak   \handler_name
    .globl  \handler_name
    .set    \handler_name, Default_Handler
    .endm

    def_irq_handler PendSV_Handler
    def_irq_handler SysTick_Handler
