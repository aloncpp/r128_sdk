obj-$(CONFIG_COMPONENTS_USB_GADGET_UACD) += uacd.o
obj-$(CONFIG_COMPONENTS_USB_GADGET_UACD) += u_audio.o
obj-$(CONFIG_COMPONENTS_USB_GADGET_UACD_WITH_AS) += u_audio_as.o

CFLAGS +=  -I components/common/aw/ringbuffer/include
ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM),y)
CFLAGS +=  -I components/common/aw/AudioSystem/include
endif
