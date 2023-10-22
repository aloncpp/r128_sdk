
obj-y += platops.o
obj-y += pm_wakesrc.o
obj-y += pm_wakeres.o
obj-y += pm_manager.o
obj-y += main.o
obj-y += pm_systeminit.o

ifeq ($(CONFIG_PM_SIMULATED_RUNNING),y)
obj-y += cpu.o
else
obj-y += load.o
endif

obj-y += pm_rpc.o

ifeq ($(CONFIG_PM_SUBSYS_RISCV_SUPPORT),y)
obj-y += driver/driver_riscv.o
endif

ifeq ($(CONFIG_PM_SUBSYS_DSP_SUPPORT),y)
obj-y += driver/driver_dsp.o
endif

obj-y += test/test_rtc.o
obj-y += test/test_wkup.o

CFLAGS += -I components/common/aw/pm/plat_m33/
CFLAGS += -I components/common/aw/pm/plat_m33/include
CFLAGS += -I components/common/aw/pm/plat_m33/firmware/include
CFLAGS += -I components/common/aw/pm/plat_m33/firmware/driver/include
