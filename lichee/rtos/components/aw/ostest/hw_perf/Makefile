obj-y += base_perf.o
obj-y += hexdump.o
obj-y += bus_perf.o
obj-y += mem_sram_perf.o
obj-y += mem_xip_perf.o
obj-y += mem_psram_perf.o
obj-y += mem_api.o
obj-y += nop_inst.o
obj-y += cache_penalty.o

CFLAGS += -I components/thirdparty/console/ \
	  -I drivers/rtos-hal/include/

ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
CFLAGS += -I arch/arm/armv8m/include/
else ifeq ($(CONFIG_ARCH_RISCV), y)
CFLAGS += -I arch/risc-v/
endif
