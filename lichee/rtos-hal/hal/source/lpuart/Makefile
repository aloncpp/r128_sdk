obj-y += hal_lpuart.o

ifeq ($(CONFIG_KERNEL_FREERTOS), y)
CFLAGS += -I $(BASE)/drivers/rtos-hal/include/hal/sdmmc/
endif

ifeq ($(CONFIG_COMPONENTS_PM), y)
CFLAGS += -Icomponents/common/aw/pm/include/
endif
