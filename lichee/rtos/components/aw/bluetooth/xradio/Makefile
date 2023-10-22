ifeq ($(CONFIG_DRIVERS_BLUETOOTH_XRADIO), y)
XRADIO_BT_OBJS += adapter/
XRADIO_BT_OBJS += framework/
endif

obj-y += cmd/

ifeq ($(CONFIG_COMPONENTS_BT_PM), y)
XRADIO_BT_OBJS += pm/
endif

ifeq ($(CONFIG_COMPONENTS_AMP_BT), y)
ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
XRADIO_BT_OBJS += manager/
obj-y += $(XRADIO_BT_OBJS)
endif #CONFIG_ARCH_RISCV_RV64
else
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
obj-y += $(XRADIO_BT_OBJS)
endif #CONFIG_ARCH_ARM_ARMV8M
endif #CONFIG_COMPONENTS_AMP_BT
