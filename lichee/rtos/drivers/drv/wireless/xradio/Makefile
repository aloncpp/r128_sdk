include components/common/aw/xradio/plt_def.mk

ifeq ($(CONFIG_DRIVER_XR819), y)
  CFLAGS += -DCONFIG_OS_NUTTX
else ifeq ($(CONFIG_DRIVER_XR829), y)
  CFLAGS += -DCONFIG_OS_NUTTX
else ifeq ($(CONFIG_DRIVER_R128), y)
  CFLAGS += -DCONFIG_OS_TINA
endif

CFLAGS += $(CFLAGS_COMMON)

ifeq ($(CONFIG_DRIVERS_XRADIO),y)
  ifeq ($(CONFIG_ETF), y)
  else
    WLAN_AMP_SER_OBJS += os/sys/mbuf/mbuf_0.o
  endif
  WLAN_AMP_SER_OBJS += driver/port/gpio.o

  ifneq ($(CONFIG_DRIVER_R128), y)
    WLAN_OBJS += driver/port/sdio.o
  endif
  #WLAN_OBJS += driver/port/delay.o
  WLAN_OBJS += driver/port/xr_drv_mem.o
  WLAN_OBJS += driver/port/ffs.o
  WLAN_OBJS += driver/port/hexdump.o
  WLAN_OBJS += driver/port/workqueue.o

  ifeq ($(CONFIG_DRIVERS_XRADIO_CMD),y)
    ifeq ($(CONFIG_ETF), y)
      WLAN_OBJS += adapter/cmd_wlan/cmd_etf.o
      WLAN_OBJS += $(WLAN_AMP_SER_OBJS)
    else
      WLAN_AMP_OBJS += ethernetif.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/utils.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/command.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_ifconfig.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_wlan.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_lmac.o
      WLAN_AMP_OBJS += adapter/cmd_wlan/cmd_wlancmd.o

      WLAN_AMP_OBJS += adapter/net_ctrl/net_ctrl.o
      WLAN_AMP_OBJS += adapter/net_ctrl/net_init.o
      WLAN_AMP_OBJS += adapter/net_ctrl/net_sys.o
      WLAN_AMP_OBJS += adapter/net_ctrl/sc_assistant_port.o

      WLAN_AMP_OBJS += adapter/sys_monitor/sys_monitor.o

      ifeq ($(CONFIG_COMPONENTS_AMP), y)
        ifeq ($(CONFIG_ARCH_RISCV_RV64), y)
          WLAN_OBJS += $(WLAN_AMP_OBJS)
        endif
        ifeq ($(CONFIG_ARCH_ARM_ARMV8M), y)
          WLAN_OBJS += $(WLAN_AMP_SER_OBJS)
        endif
      else
          WLAN_OBJS += $(WLAN_AMP_OBJS) $(WLAN_AMP_SER_OBJS)
      endif

      CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl
      CFLAGS += -I drivers/drv/wireless/xradio/adapter/sys_ctrl
      CFLAGS += -I drivers/drv/wireless/xradio/adapter/cmd_wlan
      CFLAGS += -I drivers/drv/wireless/xradio/adapter/sys_monitor
    endif
    CFLAGS += -I components/common/aw/xradio/cmd
  endif

endif

CFLAGS += -I drivers/drv/wireless/xradio/include
CFLAGS += -I drivers/drv/wireless/xradio
CFLAGS += -I drivers/drv/wireless/xradio/os
CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I include/freertos/
CFLAGS += -I projects/

CFLAGS += $(PLT_CFLAGS)

ifeq ($(CONFIG_DRIVERS_XRADIO),y)

  ifeq ($(CONFIG_DRIVER_XR819), y)
    CFLAGS += -I drivers/chip-src/sdmmc
    CFLAGS += -I include/drivers/sdmmc
  else ifeq ($(CONFIG_DRIVER_XR829), y)
    CFLAGS += -I drivers/chip-src/sdmmc
    CFLAGS += -I include/drivers/sdmmc
  else ifeq ($(CONFIG_DRIVER_R128), y)
    CFLAGS += -I drivers/rtos-hal/hal/source/prcm/prcm-sun20iw2
    CFLAGS += -I drivers/rtos-hal/include/hal
  endif

  ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
    LWIP = lwip-2.1.2
  else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
    LWIP = lwip-2.0.3
  endif

  CFLAGS += -I drivers/drv/wireless/xradio/os/include/net
  CFLAGS += -I components/common/thirdparty/network/udhcp-0.9.8/include
  CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include

  CFLAGS += -I include/drivers
  CFLAGS += -I components/common/aw/network/tcpip
  CFLAGS += -I components/thirdparty/console
  CFLAGS += -I drivers/drv/wireless/sunxi-platform

endif

obj-y +=$(WLAN_OBJS)
