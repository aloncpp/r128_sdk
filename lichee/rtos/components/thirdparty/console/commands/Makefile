obj-$(CONFIG_COMMAND_MISC) += domisc.o
obj-y += command_api.o
obj-$(CONFIG_COMMAND_FORK) += dofork.o
obj-$(CONFIG_COMMAND_EFUSE_TEST) += efuse_test.o
obj-$(CONFIG_COMMAND_TOP) += dotop.o
obj-$(CONFIG_COMMAND_MTOP) += domtop.o
obj-$(CONFIG_COMMAND_MAHB) += domahb.o
obj-$(CONFIG_COMMAND_TEST_CACHE) += test_cache.o
obj-$(CONFIG_ARCH_RISCV_PMU) += dopmu.o
obj-$(CONFIG_COMMAND_CSR) += cmd_csr.o
obj-$(CONFIG_COMMAND_DCDC_SWITCH) += cmd_dcdc_switch.o

obj-$(CONFIG_COMMAND_MEMTESTER) += memtester/
obj-$(CONFIG_COMMAND_EXEC) += doexec.o

CFLAGS += -I $(BASE)/components/thirdparty/console/

ifeq ($(CONFIG_COMPONENTS_BOOT_REASON), y)
CFLAGS += -I components/common/aw/boot_reason/
endif

CFLAGS += -D_POSIX_MONOTONIC_CLOCK
