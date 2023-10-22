obj-y += head_s.o
obj-y += plic.o
obj-y += cache.o
obj-y += spinlock.o
obj-y += clic.o
obj-y += interrupt.o

ifdef CONFIG_ARCH_RISCV_VECTOR
obj-y += memcpy.o
obj-y += memcpy_vector.o
endif


CFLAGS += -I $(BASE)/arch/risc-v/sun20iw2p1/
