obj-y += boot/head.o
obj-y += boot/spare_head.o

obj-y += common/exception.o
obj-y += common/spinlock.o
obj-y += nvic/nvic.o
obj-y += mm/mmu_cache.o

obj-$(CONFIG_ARCH_SUN20IW2) += sun20iw2p1/sun20i.o
obj-$(CONFIG_ARCH_SUN20IW2) += sun20iw2p1/timer.o
obj-$(CONFIG_ARCH_SUN20IW2) += sun20iw2p1/clocksource.o
obj-$(CONFIG_ARCH_SUN20IW2) += sun20iw2p1/cpufreq.o

CFLAGS += -I $(BASE)/include/arch/arm/armv8m/
CFLAGS += -I $(BASE)/arch/arm/armv8m/include/cmsis/
CFLAGS += -I $(BASE)/include/arch/arm/armv8m/sun20iw2p1/
CFLAGS += -I $(BASE)/include/arch/arm/mach/sun20iw2p1/

CFLAGS-$(CONFIG_COMPONENTS_BOOT_REASON) += -I $(BASE)/components/common/aw/boot_reason/
subdir-ccflags-$(CONFIG_COMPONENTS_PM) += -I $(BASE)/components/common/aw/pm/include/
