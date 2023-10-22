AW_PM_PATH     = components/common/aw/pm
AW_PM_SRCDIR   = $(AW_PM_PATH)

subdir-ccflags-y += -I$(AW_PM_SRCDIR)/
subdir-ccflags-y += -I$(AW_PM_SRCDIR)/common/
subdir-ccflags-y += -I$(AW_PM_SRCDIR)/include/

subdir-ccflags-$(CONFIG_COMPONENTS_PM_CORE_M33) += -I $(AW_PM_SRCDIR)/plat_m33/include/
subdir-ccflags-$(CONFIG_COMPONENTS_PM_CORE_DSP) += -I $(AW_PM_SRCDIR)/plat_dsp/include/
subdir-ccflags-$(CONFIG_COMPONENTS_PM_CORE_RISCV) += -I $(AW_PM_SRCDIR)/plat_riscv/include/

obj-y += common/
obj-$(CONFIG_COMPONENTS_PM_CORE_M33) += plat_m33/
obj-$(CONFIG_COMPONENTS_PM_CORE_DSP) += plat_dsp/
obj-$(CONFIG_COMPONENTS_PM_CORE_RISCV) += plat_riscv/
