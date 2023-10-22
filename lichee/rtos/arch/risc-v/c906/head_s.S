#include <csr.h>

#define EN_THEADISAEE   (0x1 << 22)
//#define CONFIG_F133_SIM 1

#define GPRCM_RV_BOOT_FLAG_REG	(0x40050000 + 0x01d8)
#define GPRCM_RV_BOOT_ADDR_REG	(0x40050000 + 0x01dc)

.section ".start", "ax", @progbits
.global	_start
.option norvc
_start:
	la    t0, GPRCM_RV_BOOT_FLAG_REG
	lw    t1, 0(t0)
	li    t0, 0x1
	bne   t1, t0, .normal
	la    t0, GPRCM_RV_BOOT_ADDR_REG
	lw    t1, 0(t0)
	jr    t1

.normal:
    # disable interrupt ability and mask all the interrupts.
#ifdef CONFIG_F133_SIM
	li    a0, 0x020000f0
	li    a1, 0x0e222222
	sw    a1, 0(a0)
#endif
    csrw  mie, zero
    csrw  mip, zero

	/*enable theadisaee*/
	li t1, EN_THEADISAEE
	csrs mxstatus, t1

	/*invaild ICACHE/DCACHE/BTB/BHT*/
	li t2, 0x30013
	csrs mcor, t2

    # x0 hardcoded to zero.
    li    x1,  0
    li    x2,  0
    li    x3,  0
    li    x4,  0
    li    x5,  0
    li    x6,  0
    li    x7,  0
    li    x8,  0
    li    x9,  0
    li    x10, 0
    li    x11, 0
    li    x12, 0
    li    x13, 0
    li    x14, 0
    li    x15, 0
    li    x16, 0
    li    x17, 0
    li    x18, 0
    li    x19, 0
    li    x20, 0
    li    x21, 0
    li    x22, 0
    li    x23, 0
    li    x24, 0
    li    x25, 0
    li    x26, 0
    li    x27, 0
    li    x28, 0
    li    x29, 0
    li    x30, 0
    li    x31, 0

    # Kind of the chicken/egg first problem
    # the initalization of gp needs to be protected from relaxation otherwise
    # the initialization of gp itself will try to use a garbage
    # gp with disastrous results.
    # without this norelax option, la gp, __global_pointer$
    # will be emitted with something like  "addi gp, gp, 0".
.option push # push the following setting of all .options to a stack. so that subsequent .option pop will restore their value.
.option norelax
    la    gp, __global_pointer$
.option pop  # pop the optios stack, restore all .options to the setting of the last .option push time.

    # auipc gp, %pcrel_hi(__global_pointer$)
    # addi  gp, gp, %pcrel_lo(1b)

    la   t0, freertos_risc_v_trap_handler
    ori  t0, t0, 0x0
    csrw mtvec, t0

1:
    /* make sure that mtvec is updated */
    csrr t1, mtvec
    bne  t1, t0, 1b

#ifdef CONFIG_ARCH_RISCV_FPU
    /* Clear and Set FS status, we are using FPU In kernel. */
    # clear SD temporaly.disable and interrupt.
    li    t0, SR_FS_DIRTY
    csrc  mstatus, t0

    # initial status to set the fpu registers.
    li    t0, SR_FS_INITIAL
    csrs  mstatus, t0

    # follwing code would change state of fpu registers
    # would set sr_sd again.

    # clear the floating-point swap control and status register.
    # abandon the original value.
    fscsr   x0, x0

    # clear the GPRS of FPU.
    fmv.d.x  f0,  x0
    fmv.d.x  f1,  x0
    fmv.d.x  f2,  x0
    fmv.d.x  f3,  x0
    fmv.d.x  f4,  x0
    fmv.d.x  f5,  x0
    fmv.d.x  f6,  x0
    fmv.d.x  f7,  x0
    fmv.d.x  f8,  x0
    fmv.d.x  f9,  x0
    fmv.d.x  f10, x0
    fmv.d.x  f11, x0
    fmv.d.x  f12, x0
    fmv.d.x  f13, x0
    fmv.d.x  f14, x0
    fmv.d.x  f15, x0
    fmv.d.x  f16, x0
    fmv.d.x  f17, x0
    fmv.d.x  f18, x0
    fmv.d.x  f19, x0
    fmv.d.x  f20, x0
    fmv.d.x  f21, x0
    fmv.d.x  f22, x0
    fmv.d.x  f23, x0
    fmv.d.x  f24, x0
    fmv.d.x  f25, x0
    fmv.d.x  f26, x0
    fmv.d.x  f27, x0
    fmv.d.x  f28, x0
    fmv.d.x  f29, x0
    fmv.d.x  f30, x0
    fmv.d.x  f31, x0

    # clear SD temporaly. and disable interrupt.
    li    t0, SR_FS_DIRTY
    csrc  mstatus, t0

    # re-initial status to initial status.
    li    t0, SR_FS_INITIAL
    csrs  mstatus, t0
#endif

#ifdef CONFIG_ARCH_RISCV_VECTOR
    li    t0, SR_VS_INITIAL
    csrs  mstatus, t0
#endif

    # clear mscratch/sscratch register.
    csrw   mscratch,zero
    csrw   sscratch,zero

    la   t0, __bss_start__
    la   t1, __bss_end__
    ble  t1, t0, clear_bss_done

clear_bss:
    sd   zero, (t0)
    add  t0, t0, 8
    blt  t0, t1, clear_bss

clear_bss_done:

    la    t0, __init_process_stack_start__
    la    t1, __init_process_stack_end__
    # fill stack with 'ebreak' instructions.
    li    t2, 0x9002900290029002

1:
    sd    t2, 0(t0)
    add   t0, t0, 8
    blt   t0, t1, 1b

    # prepare stack for c.
    la    sp, __init_process_stack_end__

    # prepare jump to c code.
.option rvc
to_c:
    # Para:
    # a0: boot cost 0 nanoseconds just for dummy
    # a1: report misa to system software.
    li   a0,  0
    csrr a1,  misa

    la    t0, start_kernel
    jalr  zero, t0
    nop
    nop

.align 3

_load_start:
    .dword 0x0

.end
