ifeq ($(CONFIG_ARCH_ARM), y)
include $(BASE)/arch/arm/armv7a/arch.mk
endif

ifeq ($(CONFIG_ARCH_RISCV), y)
include $(BASE)/arch/risc-v/arch.mk
endif
