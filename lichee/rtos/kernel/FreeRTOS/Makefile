obj-y += Source/list.o
obj-y += Source/queue.o
obj-y += Source/tasks.o
obj-y += Source/timers.o
obj-y += Source/FreeRTOS_tick_config.o
obj-y += Source/event_groups.o

obj-$(CONFIG_HEAP_4) += Source/portable/MemMang/heap_4.o
obj-$(CONFIG_RTT_SLAB) += Source/portable/MemMang/slab_port.o
obj-$(CONFIG_RTT_SLAB) += Source/portable/MemMang/slab_service.o

obj-y += Source/portable/GCC/ARM_CA9/port.o
obj-y += Source/portable/GCC/ARM_CA9/portASM.o
obj-y += Source/portable/GCC/ARM_CA9/port_misc.o

obj-$(CONFIG_RTT_SLAB) += Source/portable/MemMang/slab.o

ifeq ($(CONFIG_KASAN), y)
ifeq ($(CONFIG_KASAN_INLINE), y)
	call_threshold := 10000
else
	call_threshold := 0
endif

KASAN_SHADOW_OFFSET ?= $(CONFIG_KASAN_SHADOW_OFFSET)

CFLAGS_KASAN := -fsanitize=kernel-address \
		-fasan-shadow-offset=$(KASAN_SHADOW_OFFSET) \
		--param asan-stack=1 --param asan-globals=1 \
		--param asan-instrument-allocas=1 -fsanitize-address-use-after-scope \
		--param asan-instrumentation-with-call-threshold=$(call_threshold)

CFLAGS_REMOVE_slab.o += $(CFLAGS_KASAN)

endif

CFLAGS += -I $(BASE)/include/drivers/
CFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/include/
CFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/
CFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/portable/GCC/ARM_CA9/

ASFLAGS += -I $(BASE)/include/drivers/
ASFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/include/
ASFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/
ASFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/portable/GCC/ARM_CA9/
ASFLAGS += -I $(BASE)/arch/arm/armv7a/include/

CFLAGS += -I $(BASE)/components/thirdparty/finsh_cli/
