

obj-$(CONFIG_COMPONENTS_RPDATA) += rpdata_core.o
obj-$(CONFIG_COMPONENTS_RPDATA_DEMO) += rpdata_demo.o
obj-$(CONFIG_COMPONENTS_RPDATA_AUDIO_DEMO) += rpdata_audio.o

CFLAGS +=  -I components/common/aw/rpdata/
CFLAGS +=  -I components/common/aw/rpdata/include
CFLAGS +=  -I components/common/aw/ringbuffer/include

ifeq ($(CONFIG_COMPONENTS_RPDATA_DEMO),y)
CFLAGS += -I components/common/thirdparty/md5
endif
ifneq ($(CONFIG_AMP_RPDATA_STUB)$(CONFIG_AMP_RPDATA_SERVICE),)
CFLAGS += -I components/common/aw/amp/service/rpdata/include
endif
ifeq ($(CONFIG_COMPONENTS_RPDATA_AUDIO_DEMO),y)
CFLAGS += -I components/common/aw/AudioSystem/include
endif
