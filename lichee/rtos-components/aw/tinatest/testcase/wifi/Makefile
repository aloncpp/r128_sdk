ifeq ($(CONFIG_DRIVERS_XRADIO), y)

obj-$(CONFIG_TESTCASE_WIFION) += tt-onwifi.o
obj-$(CONFIG_TESTCASE_WIFIOFF) += tt-offwifi.o
obj-$(CONFIG_TESTCASE_WIFISCAN) += tt-scanwifi.o
obj-$(CONFIG_TESTCASE_WIFICON) += tt-conwifi.o
obj-$(CONFIG_TESTCASE_WIFIRM) += tt-rmwifi.o

CFLAGS += -I components/common/thirdparty/network/lwip/src/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter
CFLAGS += -I drivers/drv/wireless/xradio/os/include

endif
