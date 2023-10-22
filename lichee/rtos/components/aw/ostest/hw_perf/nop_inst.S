.global nop_instruction

nop_instruction:
    .rept 256
    nop
    .endr
#if defined(CONFIG_ARCH_ARM)
    bx lr
#elif defined(CONFIG_ARCH_RISCV)
    ret
#endif
