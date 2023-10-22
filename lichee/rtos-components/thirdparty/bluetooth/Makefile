obj-$(CONFIG_COMPONENTS_RTLBTSTACK) += rtlbt/
obj-$(CONFIG_COMPONENTS_ZBLUE) += zblue/

ifeq ($(CONFIG_COMPONENTS_AMP_BT), y)
ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
ifeq ($(CONFIG_COMPONENTS_BLUEDROID), y)
obj-y += bluedroid/
endif
ifeq ($(CONFIG_COMPONENTS_ZEPHYR), y)
obj-y += zephyr/
endif
ifeq ($(CONFIG_BT_DRIVERS_LOG), y)
obj-y += tools/
endif
endif #CONFIG_ARCH_RISCV_RV64
else #CONFIG_COMPONENTS_AMP_BT
ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
ifeq ($(CONFIG_COMPONENTS_BLUEDROID), y)
obj-y += bluedroid/
endif
ifeq ($(CONFIG_COMPONENTS_ZEPHYR), y)
obj-y += zephyr/
endif
ifeq ($(CONFIG_BT_DRIVERS_LOG), y)
obj-y += tools/
endif
endif #CONFIG_ARCH_ARM_ARMV8M
endif #CONFIG_COMPONENTS_AMP_BT
