
obj-$(CONFIG_COMPONENTS_OMX_SYSTEM) += OmxCore.o OmxCommon.o OMXComponentLoader.o

obj-$(CONFIG_COMPONENTS_OMX_BASE) += omx_base/omx_base_component.o
obj-$(CONFIG_COMPONENTS_OMX_BASE) += omx_base/omx_base_port.o
obj-$(CONFIG_COMPONENTS_OMX_BASE_FILTER) += omx_base/omx_base_filter.o
obj-$(CONFIG_COMPONENTS_OMX_BASE_SINK) += omx_base/omx_base_sink.o
obj-$(CONFIG_COMPONENTS_OMX_BASE_SOURCE) += omx_base/omx_base_source.o
obj-$(CONFIG_COMPONENTS_OMX_BASE_AUDIO_PORT) += omx_base/omx_base_audio_port.o
obj-$(CONFIG_COMPONENTS_OMX_BASE_CLOCK_PORT) += omx_base/omx_base_clock_port.o

ifeq ($(CONFIG_COMPONENTS_OMX_AUDIO_UTILS),y)

ifeq ($(CONFIG_COMPONENTS_RPDATA),y)
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_RECORD) += utils/arecord_test.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_RENDER) += utils/arender_test.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_ECHO_CANCEL) += utils/aec_test.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_ASR) += utils/asr_test.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_UTILS) += utils/common/rpdata_common_interface.o
endif

ifeq ($(CONFIG_COMPONENTS_RPBUF),y)
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_EQUALIZER) += utils/aeq_test.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_ECHO_CANCEL) += utils/rpbuf_aec_test.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_UTILS) += utils/common/rpbuf_common_interface.o
endif

endif

obj-$(CONFIG_COMPONENTS_OMX_AUDIO_ECHO_CANCEL) += component/aec_component/aec_component.o

obj-$(CONFIG_COMPONENTS_OMX_AUDIO_EQUALIZER) += component/aeq_component/aeq_component.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_EQUALIZER) += component/aeq_component/eq/eq.o

obj-$(CONFIG_COMPONENTS_OMX_AUDIO_ASR) += component/asr_component/asr_component.o

obj-$(CONFIG_COMPONENTS_OMX_AUDIO_NOISE_SUPPRESS) += component/ans_component/ans_component.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_RECORD) += component/arecord_component/arecord_component.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_RENDER) += component/arender_component/arender_component.o
obj-$(CONFIG_COMPONENTS_OMX_AUDIO_EFFECT) += component/aeffect_component/aeffect_component.o
obj-$(CONFIG_COMPONENTS_OMX_DUMP) += component/dump_component/dump_component.o

CFLAGS +=  -I components/common/aw/OpenMax/
CFLAGS +=  -I components/common/aw/OpenMax/include
CFLAGS +=  -I components/common/aw/OpenMax/component
CFLAGS +=  -I components/common/aw/OpenMax/omx_base
CFLAGS +=  -I components/common/aw/OpenMax/component/aec_component
CFLAGS +=  -I components/common/aw/OpenMax/component/aeq_component
CFLAGS +=  -I components/common/aw/OpenMax/component/aeq_component/eq
CFLAGS +=  -I components/common/aw/OpenMax/component/asr_component
CFLAGS +=  -I components/common/aw/OpenMax/component/arecord_component
CFLAGS +=  -I components/common/aw/OpenMax/component/arender_component
CFLAGS +=  -I components/common/aw/OpenMax/component/dump_component
CFLAGS +=  -I components/common/aw/OpenMax/utils/
CFLAGS +=  -I components/common/aw/OpenMax/utils/common

CFLAGS +=  -I components/common/aw/asr_demo/inc/

ifeq ($(CONFIG_COMPONENTS_RINGBUFFER),y)
CFLAGS +=  -I components/common/aw/ringbuffer/include
endif
ifeq ($(CONFIG_COMPONENTS_RPBUF),y)
CFLAGS +=  -I components/common/aw/rpbuf/include
endif
ifeq ($(CONFIG_COMPONENTS_RPDATA),y)
CFLAGS +=  -I components/common/aw/rpdata/include
endif
ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM),y)
CFLAGS += -I components/common/aw/AudioSystem/include
endif

ifeq ($(CONFIG_COMPONENTS_PROCESS_AEC),y)
CFLAGS +=  -I components/common/aw/process_module/aec/include/
ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
CFLAGS +=  -D__ARMEL__
else ifeq ($(CONFIG_ARCH_RISCV_RV64),y)
CFLAGS +=  -D__RV64__
else ifeq ($(CONFIG_ARCH_DSP),y)
CFLAGS +=  -D__DSP__
endif
endif

ifeq ($(CONFIG_COMPONENTS_PROCESS_ASR),y)
CFLAGS +=  -I components/common/aw/process_module/asr/include/
ifeq ($(CONFIG_ARCH_ARM_CORTEX_A7),y)
CFLAGS +=  -D__ARMEL__
else ifeq ($(CONFIG_ARCH_RISCV_RV64),y)
CFLAGS +=  -D__RV64__
else ifeq ($(CONFIG_ARCH_DSP),y)
CFLAGS +=  -D__DSP__
endif
endif
