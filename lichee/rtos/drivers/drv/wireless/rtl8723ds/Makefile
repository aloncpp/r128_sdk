obj-y += os/customer_rtos/customer_rtos_service.o
obj-y += os/os_dep/net_stack_intf.o
obj-y += os/os_dep/osdep_service.o
obj-y += os/os_dep/device_lock.o
obj-y += platform/rtwlan_bsp/rtwlan_bsp.o
obj-y += platform/sdio/wifi_io.o
obj-y += api/wifi_interactive_mode.o
obj-y += api/wifi/wifi_util.o
obj-y += api/wifi/wifi_conf.o
obj-y += api/wifi/wifi_ind.o
obj-y += api/wifi/wifi_promisc.o

CFLAGS += -I $(BASE)/components/common/thirdparty/network/lwip/src/include
CFLAGS += -I $(BASE)/drivers/chip-src/sdmmc
CFLAGS += -I $(BASE)/include/drivers/chip

CFLAGS += -I $(BASE)/components/common/aw/network/tcpip
CFLAGS += -I $(BASE)/components/thirdparty/console
CFLAGS += -I $(BASE)/drivers/drv/wireless/sunxi-platform
CFLAGS += -I $(BASE)/drivers/hal/include

RTL8723DS_CFLAGS_DIR:= drivers/drv/wireless/rtl8723ds

CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/include
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/os/os_dep/include
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/platform/include
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/platform/sdio/include
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/platform/rtwlan_bsp
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/os/customer_rtos
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/api
CFLAGS += -I $(RTL8723DS_CFLAGS_DIR)/platform/sdio

obj-y += wlan_lib/libwlan_lib.o

CFLAGS += -DCONFIG_PLATFOMR_CUSTOMER_RTOS -DCONFIG_HARDWARE_8723D -DCONFIG_WLAN -DCONFIG_LWIP_LAYER
