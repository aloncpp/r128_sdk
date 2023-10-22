SPEEX_DIR :=$(BASE)/components/thirdparty/speex

CFLAGS += -I $(SPEEX_DIR)/include \
        -I $(BASE)/components/thirdparty/finsh_cli \
        -I $(BASE)/include \
        -I $(BASE)/components/thirdparty/elmfat/src \
        -I $(BASE)/components/thirdparty/ulog

obj-$(CONFIG_COMPONENT_SPEEX) += test/test_speex.o
obj-$(CONFIG_COMPONENT_SPEEX) += speex_decode.o
obj-$(CONFIG_COMPONENT_SPEEX) += speex_encode.o


