obj-y += common/exception.o
obj-y += common/clocksource.o
obj-$(CONFIG_DEBUG_BACKTRACE) += common/backtrace.o

obj-$(CONFIG_ARCH_RISCV_C906) += c906/
obj-$(CONFIG_ARCH_RISCV_E906) += e906/

obj-$(CONFIG_ARCH_SUN20IW2) += sun20iw2p1/
obj-$(CONFIG_ARCH_SUN55IW3) += sun55iw3p1/
obj-$(CONFIG_ARCH_SUN60IW1) += sun60iw1p1/

CFLAGS-$(CONFIG_ARCH_SUN20IW2) += -I $(BASE)/arch/risc-v/sun20iw2p1/
CFLAGS-$(CONFIG_ARCH_SUN55IW3) += -I $(BASE)/arch/risc-v/sun55iw3p1/
CFLAGS-$(CONFIG_ARCH_SUN60IW1) += -I $(BASE)/arch/risc-v/sun60iw1p1/
CFLAGS-$(CONFIG_ARCH_RISCV_C906) += -I $(BASE)/arch/risc-v/c906/

CFLAGS-$(CONFIG_COMPONENTS_BOOT_REASON) += -I $(BASE)/components/common/aw/boot_reason/
CFLAGS-$(CONFIG_COMPONENTS_KALLSYMS) += -I $(BASE)/components/common/aw/kallsyms/include/
