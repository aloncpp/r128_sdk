obj-$(CONFIG_COMPONENTS_LPVOICE) += lpvoice_service.o
obj-$(CONFIG_COMPONENT_UVOICE_ASR) += lpv_asr_uvoice.o

ifeq ($(CONFIG_COMPONENT_UVOICE_ASR),y)
CFLAGS += -I components/common/thirdparty/uvoice/asr/include/
endif

CFLAGS += -I components/common/aw/AudioSystem/include
CFLAGS += -I components/common/aw/pm/include/
