obj-y += dummy.o

ifeq ($(CONFIG_KERNEL_FREERTOS), y)
obj-$(CONFIG_ARCH_SUN8IW20) += libfreertos-nand-sun8iw20.o

ifeq ($(CONFIG_ARCH_RISCV)_$(CONFIG_DRIVERS_NAND_FLASH), y_y)
obj-$(CONFIG_ARCH_SUN20IW2) += libfreertos-nand-sun20iw2-c906.o
else ifeq ($(CONFIG_ARCH_ARM)_$(CONFIG_DRIVERS_NAND_FLASH), y_y)
obj-$(CONFIG_ARCH_SUN20IW2) += libfreertos-nand-sun20iw2-m33.o
endif

endif
