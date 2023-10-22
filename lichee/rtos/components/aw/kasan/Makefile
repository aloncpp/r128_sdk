obj-y += kasan.o
obj-y += freertos.o
#obj-y += hexdump.o
obj-y += report.o

CFLAGS += -fno-conserve-stack -fno-stack-protector

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

CFLAGS_REMOVE += $(CFLAGS_KASAN)
CFLAGS_REMOVE += -pg
