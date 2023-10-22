obj-y += src/main.o
obj-y += src/assert.o
obj-y += src/hooks.o
obj-y += src/alsa_config.o
obj-y += src/card_default.o

ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM),y)
CFLAGS += -I $(BASE)/components/common/aw/AudioSystem/include
endif

ifeq ($(CONFIG_COMPONENTS_WLAN_CSFC),y)
CFLAGS += -I $(BASE)/components/common/aw/network/wlan_csfc
endif
