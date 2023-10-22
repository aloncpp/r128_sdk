HCIOPTION = H4

obj-y += bt_test.o
obj-y += shell/shell_bt.o
obj-y += shell/shell_gatt.o

obj-$(CONFIG_AW_BTETF_TOOL) += bt_etf.o
obj-$(CONFIG_AW_BTETF_TOOL) += lib/libbtetf.o

CFLAGS +=-DCONFIG_ARM -DZTEST_UNITTEST -D$(HCIOPTION)
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/include
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/include/bluetooth
CFLAGS += -I drivers/drv/wireless/xradio/os/include
