obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW) += audio_hw/audio_hw.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_PCM) += audio_hw/as_pcm.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AMP) += audio_hw/as_amp.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_BT) += audio_hw/as_bt.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_USB) += audio_hw/as_usb.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AW_EQ) += audio_hw/as_equal.o

ifneq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_EQ)$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_DRC),)
obj-y += audio_hw/as_eq_drc.o
endif

obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS) += utils/as_test.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS_LOOP) += utils/as_loop.o

obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN) += audio_plugin/audio_plugin.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN) += audio_plugin/channels_map.o audio_plugin/softvolume.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN) += audio_plugin/bits_convert.o
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN) += audio_plugin/resample/resample.o

ifneq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_PLUGIN),y)
obj-$(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM) += audio_plugin/empty_audio_plugin.o
endif

ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW),y)
obj-$(CONFIG_DRIVER_SYSCONFIG) += audio_hw/ahw_config.o
endif

CFLAGS +=  -I components/common/aw/AudioSystem/
CFLAGS +=  -I components/common/aw/AudioSystem/include
CFLAGS +=  -I components/common/aw/AudioSystem/audio_plugin
CFLAGS +=  -I drivers/rtos-hal/hal/source/sound/common
CFLAGS +=  -I components/common/aw/aactd-lib/include/
CFLAGS +=  -I components/common/thirdparty/speexdsp/

ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_BT),y)
CFLAGS +=  -I components/common/aw/wireless/btmanager/src
CFLAGS +=  -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS +=  -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS +=  -I components/common/aw/xradio/include
CFLAGS +=  -I components/common/aw/ringbuffer/include
endif

ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_HW_AMP),y)
CFLAGS +=  -I components/common/aw/amp/service/audio/include
endif

ifeq  ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM_UTILS_LOOP_AEC),y)
CFLAGS +=  -I components/common/aw/process_module/aec/include/

ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
CFLAGS +=  -D__ARMEL__
else ifeq ($(CONFIG_ARCH_RISCV_RV64),y)
CFLAGS +=  -D__RV64__
else ifeq ($(CONFIG_ARCH_DSP),y)
CFLAGS +=  -D__DSP__
endif

endif

ifeq ($(CONFIG_COMPONENTS_PROCESS_EQ),y)
CFLAGS +=  -I components/common/aw/process_module/eq/include/
ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
CFLAGS +=  -D__ARMEL__
else ifeq ($(CONFIG_ARCH_RISCV_RV64),y)
CFLAGS +=  -D__RV64__
else ifeq ($(CONFIG_ARCH_DSP),y)
CFLAGS +=  -D__DSP__
endif
endif

ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
obj-y += libaudiosystem_a7.o
else ifeq ($(CONFIG_ARCH_ARM_CORTEX_M33),y)
obj-y += libaudiosystem_m33.o
else ifeq ($(CONFIG_ARCH_RISCV),y)
obj-y += libaudiosystem_c906.o
else ifeq ($(CONFIG_XTENSA_HIFI4),y)
obj-y += libaudiosystem_hifi4.o
else ifeq ($(CONFIG_XTENSA_HIFI5),y)
obj-y += libaudiosystem_hifi5.o
endif


