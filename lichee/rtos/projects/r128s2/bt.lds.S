
	*build/RTOS_TARGET_PROJECT_PATH/components/aw/bluetooth/xradio/framework/bt_ctrl.o (.text .text.* .rodata .rodata.*)

#if CONFIG_COMPONENTS_BLUEDROID
	*build/RTOS_TARGET_PROJECT_PATH/components/common/thirdparty/bluetooth/bluedroid*.o (.text .text.* .rodata .rodata.*)

	*build/RTOS_TARGET_PROJECT_PATH/components/aw/bluetooth/xradio/cmd/bluedroid*.o (.text .text.* .rodata .rodata.*)
	*build/RTOS_TARGET_PROJECT_PATH/components/aw/bluetooth/xradio/adapter/bluedroid*.o (.text .text.* .rodata .rodata.*)
#endif

#if CONFIG_COMPONENTS_ZEPHYR
	*build/RTOS_TARGET_PROJECT_PATH/components/common/thirdparty/bluetooth/zephyr*.o (.text .text.* .rodata .rodata.*)

	*build/RTOS_TARGET_PROJECT_PATH/components/aw/bluetooth/xradio/cmd/zephyr*.o (.text .text.* .rodata .rodata.*)
	*build/RTOS_TARGET_PROJECT_PATH/components/aw/bluetooth/xradio/adapter/zephyr*.o (.text .text.* .rodata .rodata.*)
#endif

#if CONFIG_BT_DRIVERS_LOG_BTSNOOP
	*build/RTOS_TARGET_PROJECT_PATH/components/common/thirdparty/bluetooth/tools/btsnoop*.o (.text .text.* .rodata .rodata.*)
#endif

#if CONFIG_BT_DRIVERS_LOG_HCIDUMP
	*build/RTOS_TARGET_PROJECT_PATH/components/common/thirdparty/bluetooth/tools/hcidump*.o (.text .text.* .rodata .rodata.*)
#endif

