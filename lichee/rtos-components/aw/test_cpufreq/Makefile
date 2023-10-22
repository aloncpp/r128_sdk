obj-$(CONFIG_COMPONENTS_TEST_CPUFREQ) += test_cpufreq.o

ifeq ($(CONFIG_ARCH_DSP), y)
CFLAGS += -Iarch/sun20iw2/
else ifeq ($(CONFIG_ARCH_RISCV), y)
CFLAGS += -Iarch/risc-v/sun20iw2p1/
else
CFLAGS += -Iarch/arm/armv8m/sun20iw2p1/
endif
