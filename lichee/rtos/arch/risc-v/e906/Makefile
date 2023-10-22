obj-y += head_s.o
obj-y += vectors.o
obj-y += cache.o
obj-y += spinlock.o
obj-y += ck_irq.o
obj-y += system.o
obj-$(CONFIG_ARCH_SUN55IW3) += platform/platform_sun55iw3.o
obj-$(CONFIG_ARCH_SUN60IW1) += platform/platform_sun60iw1.o

CFLAGS-$(CONFIG_ARCH_SUN55IW3) += -I $(BASE)/arch/risc-v/sun55iw3p1/
CFLAGS-$(CONFIG_ARCH_SUN60IW1) += -I $(BASE)/arch/risc-v/sun60iw1p1/
