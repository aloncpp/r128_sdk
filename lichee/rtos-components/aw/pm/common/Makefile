obj-y += pm_base.o
obj-y += pm_testlevel.o
obj-y += pm_devops.o
obj-y += pm_notify.o
obj-y += pm_platops.o
obj-y += pm_subsys.o
obj-y += pm_syscore.o
obj-y += pm_task.o
obj-y += pm_wakecnt.o
obj-y += pm_wakelock.o
obj-y += pm_state.o
obj-y += pm_suspend.o
obj-y += pm_wakesrc.o

ifeq ($(CONFIG_COMPONENTS_PM_CORE_M33), y)
CFLAGS += -I components/common/aw/pm/plat_m33/include
endif

ifeq ($(CONFIG_COMPONENTS_PM_CORE_DSP), y)
CFLAGS += -I components/common/aw/pm/plat_dsp/include
endif

ifeq ($(CONFIG_COMPONENTS_PM_CORE_RISCV), y)
CFLAGS += -I components/common/aw/pm/plat_riscv/include
endif

CFLAGS += -I components/common/aw/pm/common/
CFLAGS += -I components/common/aw/pm/include/
CFLAGS += -I components/common/aw/virt_log/
