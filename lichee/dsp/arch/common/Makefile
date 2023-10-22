obj-y += main.o
obj-y += idle_task.o
obj-y += start_task.o
obj-y += console_uart.o
obj-y += arch_init.o
obj-y += sys_config.o

obj-$(CONFIG_DRIVERS_MSGBOX_AMP) += mailbox_heartbeat.o

CFLAGS += -I components/common/thirdparty/openamp/include

ifeq ($(CONFIG_COMPONENTS_PM), y)
CFLAGS += -Icomponents/common/aw/pm/include/
endif

CFLAGS += -DSYSCONFG_BIN_PATH=\"$(LICHEE_CHIP_OUT_DIR)/sys_config.bin\"
