obj-$(CONFIG_DRIVERS_RFKILL) += rfkill/
obj-$(CONFIG_DRIVERS_CPUFREQ) += cpufreq/
obj-$(CONFIG_DRIVERS_DRAM) += dram/
obj-$(CONFIG_DRIVERS_FDCM) += fdcm/

ifeq ($(CONFIG_DRIVERS_WIRELESS), y)
obj-y += wireless/
else ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID), y)
obj-y += wireless/
else ifeq ($(CONFIG_AW_COMPONENT_BLUEDROID_CTRL), y)
obj-y += wireless/
endif

obj-$(CONFIG_DRIVERS_LEDS) += leds/
obj-$(CONFIG_DRIVERS_INPUT) += input/
obj-$(CONFIG_DRIVERS_MUTEKEY) += mutekey/
obj-$(CONFIG_DRIVERS_UART_DEVFS) += uart/
obj-y += bluetooth/

subdir-ccflags-y += -I components/common/aw/pm/include
subdir-ccflags-y += -I include/drivers/
subdir-ccflags-y += -I include/arch/mach
subdir-ccflags-y += -I include/freertos/
