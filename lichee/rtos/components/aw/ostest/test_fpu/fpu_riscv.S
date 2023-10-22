/*void set_fpu_register_test(void *src, void *out, int count, int reserved);*/
.global set_fpu_register_test
.extern fpu_test_thread_sleep

set_fpu_register_test:
    addi sp, sp, -32
#if __riscv_xlen == 64
    sd   ra, 24(sp)
#else
    sw   ra, 28(sp)
#endif
    flw  f0, 0(a0)
    flw  f1, 1 * 4(a0)
    flw  f2, 2 * 4(a0)
    flw  f3, 3 * 4(a0)
    flw  f4, 4 * 4(a0)
    flw  f5, 5 * 4(a0)
    flw  f6, 6 * 4(a0)
    flw  f7, 7 * 4(a0)
    flw  f8, 8 * 4(a0)
    flw  f9, 9 * 4(a0)
    flw  f10, 10 * 4(a0)
    flw  f11, 11 * 4(a0)
    flw  f12, 12 * 4(a0)
    flw  f13, 13 * 4(a0)
    flw  f14, 14 * 4(a0)
    flw  f15, 15 * 4(a0)
    flw  f16, 16 * 4(a0)
    flw  f17, 17 * 4(a0)
    flw  f18, 18 * 4(a0)
    flw  f19, 19 * 4(a0)
    flw  f20, 20 * 4(a0)
    flw  f21, 21 * 4(a0)
    flw  f22, 22 * 4(a0)
    flw  f23, 23 * 4(a0)
    flw  f24, 24 * 4(a0)
    flw  f25, 25 * 4(a0)
    flw  f26, 26 * 4(a0)
    flw  f27, 27 * 4(a0)
    flw  f28, 28 * 4(a0)
    flw  f29, 29 * 4(a0)
    flw  f30, 30 * 4(a0)
    flw  f31, 31 * 4(a0)
1:
    add   a3, a3, 1
    blt   a3, a2, 1b

#if __riscv_xlen == 64
    sd    a0, 8(sp)
    sd    a1, 16(sp)
#else
    sw    a0, 4(sp)
    sw    a1, 8(sp)
#endif
	call  fpu_test_thread_sleep
#if __riscv_xlen == 64
    ld    a0, 8(sp)
    ld    a1, 16(sp)
#else
    lw    a0, 4(sp)
    lw    a1, 8(sp)
#endif

    fsw  f0, 0(a1)
    fsw  f1, 1 * 4(a1)
    fsw  f2, 2 * 4(a1)
    fsw  f3, 3 * 4(a1)
    fsw  f4, 4 * 4(a1)
    fsw  f5, 5 * 4(a1)
    fsw  f6, 6 * 4(a1)
    fsw  f7, 7 * 4(a1)
    fsw  f8, 8 * 4(a1)
    fsw  f9, 9 * 4(a1)
    fsw  f10, 10 * 4(a1)
    fsw  f11, 11 * 4(a1)
    fsw  f12, 12 * 4(a1)
    fsw  f13, 13 * 4(a1)
    fsw  f14, 14 * 4(a1)
    fsw  f15, 15 * 4(a1)
    fsw  f16, 16 * 4(a1)
    fsw  f17, 17 * 4(a1)
    fsw  f18, 18 * 4(a1)
    fsw  f19, 19 * 4(a1)
    fsw  f20, 20 * 4(a1)
    fsw  f21, 21 * 4(a1)
    fsw  f22, 22 * 4(a1)
    fsw  f23, 23 * 4(a1)
    fsw  f24, 24 * 4(a1)
    fsw  f25, 25 * 4(a1)
    fsw  f26, 26 * 4(a1)
    fsw  f27, 27 * 4(a1)
    fsw  f28, 28 * 4(a1)
    fsw  f29, 29 * 4(a1)
    fsw  f30, 30 * 4(a1)
    fsw  f31, 31 * 4(a1)
#if __riscv_xlen == 64
    ld   ra, 24(sp)
#else
    lw   ra, 28(sp)
#endif
    addi sp, sp, 32
    ret

