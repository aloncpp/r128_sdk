###complie wifimanger core part####################################################################################

ifeq ($(CONFIG_COMPONENTS_WIFIMANAGER), y)
WIFIMG_CORE_OBJS += core/src/wifimg.o
WIFIMG_CORE_OBJS += core/src/wmg_common.o
WIFIMG_CORE_OBJS += core/src/log/wifi_log.o
WIFIMG_DEMO_OBJS += demo/wifi.o
WIFIMG_CORE_OBJS += core/src/os/freertos/freertos_common.o
ifeq ($(CONFIG_WMG_SUPPORT_EXPAND), y)
WIFIMG_DEMO_OBJS += core/src/expand_cmd.o
WIFIMG_DEMO_OBJS += core/src/os/freertos/expand/freertos_expand_cmd.o
endif
WIFIMG_DEMO_OBJS += demo/wifi_daemon.o

CFLAGS += $(if $(CONFIG_WMG_DEFAULT_DEBUG_LV_ERROR),-DDEFAULT_DEBUG_LV_ERROR)
CFLAGS += $(if $(CONFIG_WMG_DEFAULT_DEBUG_LV_WARN),-DDEFAULT_DEBUG_LV_WARN)
CFLAGS += $(if $(CONFIG_WMG_DEFAULT_DEBUG_LV_INFO),-DDEFAULT_DEBUG_LV_INFO)
CFLAGS += $(if $(CONFIG_WMG_DEFAULT_DEBUG_LV_DEBUG),-DDEFAULT_DEBUG_LV_DEBUG)
CFLAGS += $(if $(CONFIG_WMG_DEFAULT_DEBUG_LV_DUMP),-DDEFAULT_DEBUG_LV_DUMP)
CFLAGS += $(if $(CONFIG_WMG_DEFAULT_DEBUG_LV_EXCE),-DDEFAULT_DEBUG_LV_EXCE)

CFLAGS += $(if $(CONFIG_WMG_SUPPORT_STA_MODE),-DSUPPORT_STA_MODE)
CFLAGS += $(if $(CONFIG_WMG_SUPPORT_AP_MODE),-DSUPPORT_AP_MODE)
CFLAGS += $(if $(CONFIG_WMG_SUPPORT_MONITOR_MODE),-DSUPPORT_MONITOR_MODE)
CFLAGS += $(if $(CONFIG_WMG_SUPPORT_P2P_MODE),-DSUPPORT_P2P_MODE)

CFLAGS += $(if $(CONFIG_WMG_SUPPORT_EXPAND),-DSUPPORT_EXPAND)
CFLAGS += $(if $(CONFIG_WMG_PLATFORM_FREERTOS),-DOS_NET_FREERTOS_OS)

ifeq ($(CONFIG_WMG_SUPPORT_STA_MODE), y)
WIFIMG_CORE_OBJS += core/src/wmg_sta.o
WIFIMG_CORE_OBJS += core/src/os/freertos/freertos_sta.o
endif

ifeq ($(CONFIG_WMG_SUPPORT_AP_MODE), y)
WIFIMG_CORE_OBJS += core/src/wmg_ap.o
WIFIMG_CORE_OBJS += core/src/os/freertos/freertos_ap.o
endif

ifeq ($(CONFIG_WMG_SUPPORT_MONITOR_MODE), y)
WIFIMG_CORE_OBJS += core/src/wmg_monitor.o
WIFIMG_CORE_OBJS += core/src/os/freertos/freertos_monitor.o
endif

ifeq ($(CONFIG_WMG_SUPPORT_P2P_MODE), y)
WIFIMG_CORE_OBJS += core/src/wmg_p2p.o
endif

ifeq ($(CONFIG_WMG_UNREGISTER_CB), y)
CFLAGS += -DUNREGISTER_CB
endif
endif
###################################################################################################################

###complie wifimanger peiwang part#################################################################################
ifneq ($(CONFIG_WMG_PROTOCOL_BLE)$(CONFIG_WMG_PROTOCOL_SOFTAP)$(CONFIG_WMG_PROTOCOL_XCONFIG)$(CONFIG_WMG_PROTOCOL_SOUNDWAVE), )
WIFIMG_CORE_OBJS += core/src/linkd.o
CFLAGS += -DSUPPORT_LINKD
endif

ifeq ($(CONFIG_WMG_PROTOCOL_SOFTAP), y)
WIFIMG_SOFTAP_OBJS += core/src/os/freertos/linkd/linkd_softap/linkd_softap.o
CFLAGS += -I components/aw/smartlink/soft_ap_config/include
endif

ifeq ($(CONFIG_WMG_PROTOCOL_BLE), y)
WIFIMG_BLE_OBJS += core/src/os/freertos/linkd/linkd_ble/linkd_ble.o
endif

ifeq ($(CONFIG_WMG_PROTOCOL_SOUNDWAVE), y)
WIFIMG_SOUNDWAVE_OBJS += core/src/os/freertos/linkd/linkd_soundwave/linkd_soundwave.o
endif

ifeq ($(CONFIG_WMG_PROTOCOL_XCONFIG), y)
	WIFIMG_XCONFIG_OBJS += core/src/os/freertos/linkd/linkd_xconfig/linkd_xconfig.o
endif
###################################################################################################################

ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
	LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
	LWIP = lwip-2.0.3
endif

CFLAGS += -D__WMGFILE__='"$(subst $(dir $<),,$<)"'

obj-y += $(WIFIMG_CORE_OBJS)
obj-y += $(WIFIMG_DEMO_OBJS)
obj-y += $(WIFIMG_BLE_OBJS)
obj-y += $(WIFIMG_SOFTAP_OBJS)
obj-y += $(WIFIMG_SOUNDWAVE_OBJS)
obj-y += $(WIFIMG_XCONFIG_OBJS)

CFLAGS += -I components/common/aw/wireless/common/src/api_action
CFLAGS += -I components/common/aw/wireless/common/src/tracing
CFLAGS += -I components/common/aw/wireless/common/src/os
CFLAGS += -I components/common/aw/wireless/common/src/os/freertos
CFLAGS += -I components/common/aw/wireless/wifimanager/core/include
CFLAGS += -I components/common/aw/wireless/wifimanager/core/include/os
CFLAGS += -I components/common/aw/wireless/wifimanager/core/include/os/freertos
CFLAGS += -I components/common/aw/wireless/wifimanager/core/src/os/freertos
CFLAGS += -I components/common/aw/wireless/wifimanager/core/src/os/freertos/lib
CFLAGS += -I components/common/aw/wireless/wifimanager/demo
CFLAGS += -I components/common/aw/wireless/btmanager/src
CFLAGS += -I components/aw/smartlink/smartconfig/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl
CFLAGS += -I drivers/drv/wireless/xradio/adapter/cmd
CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I drivers/drv/wireless/xradio/include/net
CFLAGS += -I drivers/drv/wireless/xradio/include/net/ethernetif
CFLAGS += -I drivers/drv/wireless/xradio/include/net/wlan
CFLAGS += -I drivers/drv/wireless/xradio/include/smartlink
CFLAGS += -I drivers/drv/wireless/xradio/lib/sun20iw2
CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I drivers/drv/wireless/xradio/os/include/net/wlan
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include
CFLAGS += -I components/common/thirdparty/bluetooth/zephyr/include/ble
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/compat/posix
CFLAGS += -I components/common/aw/network/tcpip
CFLAGS += -I components/common/aw/xradio/cmd
CFLAGS += -I components/common/aw/xradio/include
CFLAGS += -I components/common/aw/xradio/include/libc
CFLAGS += -I components/common/aw/xradio/include/sysinfo
