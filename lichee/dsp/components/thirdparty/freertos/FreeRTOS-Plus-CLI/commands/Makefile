obj-y += aw_cmd.o
obj-y += command_api.o domisc.o
obj-$(CONFIG_FREERTOS_CLI_CMD_TEMPLATE) += cmd_template.o
obj-$(CONFIG_FREERTOS_CLI_CMD_MEM_RW) += cmd_md.o cmd_mw.o
obj-$(CONFIG_FREERTOS_CLI_CMD_FREE) += cmd_free.o
obj-$(CONFIG_FREERTOS_CLI_CMD_FORK) += dofork.o
obj-$(CONFIG_FREERTOS_CLI_CMD_TOP) += cmd_top.o
obj-$(CONFIG_FREERTOS_CLI_CMD_TEST_CACHE) += test_cache.o

CFLAGS_domisc.o += -Icomponents/thirdparty/freertos/FreeRTOS-Plus-CLI/
