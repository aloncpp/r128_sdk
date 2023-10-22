obj-y += boot/head.o
obj-y += boot/spare_head.o
obj-y += common/memcpy-armv7a.o
obj-y += mm/armv7a.o
obj-y += mm/cp15_gcc.o
obj-y += gic/gic.o
obj-y += gic/ns-gic.o
obj-y += arch_timer/generic_timer.o
obj-y += sun8i/sun8i.o
obj-y += sun8i/clocksource.o

obj-$(CONFIG_ARCH_SUN8IW18) += sun8i/sun8iw18_secondary.o
obj-$(CONFIG_ARCH_SUN8IW20) += sun8i/sun8iw20_secondary.o

obj-y += common/spinlock.o
obj-y += common/vector.o
obj-y += mm/cpux_mmu.o
obj-y += mm/mmu_cache.o

ifeq ($(CONFIG_KASAN), y)
ifeq ($(CONFIG_KASAN_INLINE), y)
	call_threshold := 10000
else
	call_threshold := 0
endif

obj-y += sun8i/kasan_init.o

KASAN_SHADOW_OFFSET ?= $(CONFIG_KASAN_SHADOW_OFFSET)

CFLAGS_KASAN := -fsanitize=kernel-address \
		-fasan-shadow-offset=$(KASAN_SHADOW_OFFSET) \
		--param asan-stack=1 --param asan-globals=1 \
		--param asan-instrument-allocas=1 -fsanitize-address-use-after-scope \
		--param asan-instrumentation-with-call-threshold=$(call_threshold)

CFLAGS_REMOVE_spinlock.o += $(CFLAGS_KASAN)
CFLAGS_REMOVE_vector.o += $(CFLAGS_KASAN)
CFLAGS_REMOVE_cpux_mmu.o += $(CFLAGS_KASAN)
CFLAGS_REMOVE_mmu_cache.o += $(CFLAGS_KASAN)
CFLAGS_REMOVE_kasan_init.o += $(CFLAGS_KASAN)
CFLAGS_REMOVE_sun8iw18_secondary.o += $(CFLAGS_KASAN)
CFLAGS_REMOVE_sun8iw20_secondary.o += $(CFLAGS_KASAN)

endif

CFLAGS += -I $(BASE)/include/arch/arm/armv7a/
CFLAGS += -I $(BASE)/include/arch/arm/armv7a/sun8i/

ASFLAGS += -I $(BASE)/include/arch/arm/armv7a/
ASFLAGS += -I $(BASE)/include/arch/arm/armv7a/sun8i/
ASFLAGS += -I $(BASE)/include/arch/arm/armv7a/include/

