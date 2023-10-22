ifeq ($(CONFIG_BLEHOST), y)
USE_BLE_HOST_MESH = n
HCIOPTION = H4

ZBLUE_OBJS += common/atomic_c.o
ZBLUE_OBJS += common/rpa.o
ZBLUE_OBJS += common/log.o

ifeq ($(CONFIG_BT_HCI_RAW), y)
ZBLUE_OBJS += host/hci_raw.o
endif

ifeq ($(CONFIG_BT_TINYCRYPT_ECC), y)
ZBLUE_OBJS += host/hci_ecc.o
endif

ifeq ($(CONFIG_BT_SETTINGS), y)
ZBLUE_OBJS += host/settings.o
endif

ifeq ($(CONFIG_BT_HOST_CCM), y)
ZBLUE_OBJS += host/aes_ccm.o
endif
ifeq ($(CONFIG_BT_HCI_HOST), y)
ZBLUE_OBJS += host/uuid.o
ZBLUE_OBJS += host/hci_core.o

  ifeq ($(CONFIG_BT_HOST_CRYPTO), y)
    ZBLUE_OBJS += host/crypto.o
  endif

  ifeq ($(CONFIG_BT_CONN), y)
    ZBLUE_OBJS += host/conn.o
    ZBLUE_OBJS += host/l2cap.o
    ZBLUE_OBJS += host/att.o
    ZBLUE_OBJS += host/gatt.o

    ifeq ($(CONFIG_BT_SMP), y)
      ZBLUE_OBJS += host/smp.o
      ZBLUE_OBJS += host/keys.o
    else
      ZBLUE_OBJS += host/smp_null.o
    endif
  endif
endif

ifeq ($(CONFIG_BT_MESH), y)
ZBLUE_OBJS += host/mesh/main.o
ZBLUE_OBJS += host/mesh/adv.o
ZBLUE_OBJS += host/mesh/beacon.o
ZBLUE_OBJS += host/mesh/net.o
ZBLUE_OBJS += host/mesh/transport.o
ZBLUE_OBJS += host/mesh/crypto.o
ZBLUE_OBJS += host/mesh/access.o
ZBLUE_OBJS += host/mesh/cfg_srv.o
ZBLUE_OBJS += host/mesh/health_srv.o
ZBLUE_OBJS += host/mesh/generic_onoff_cli.o
ZBLUE_OBJS += host/mesh/generic_onoff_srv.o
ZBLUE_OBJS += host/mesh/mesh_common.o
ZBLUE_OBJS += host/mesh/transition.o
endif

ifeq ($(CONFIG_BT_MESH_LOW_POWER), y)
ZBLUE_OBJS += host/mesh/lpn.o
endif

ifeq ($(CONFIG_BT_MESH_FRIEND), y)
ZBLUE_OBJS += host/mesh/friend.o
endif

ifeq ($(CONFIG_BT_MESH_PROV), y)
ZBLUE_OBJS += host/mesh/prov.o
endif

ifeq ($(CONFIG_BT_MESH_PB_ADV), y)
ZBLUE_OBJS += host/pb_adv.o
endif

ifeq ($(CONFIG_BT_MESH_PB_GATT), y)
ZBLUE_OBJS += host/pb_gatt.o
endif

ifeq ($(CONFIG_BT_MESH_PROXY), y)
ZBLUE_OBJS += host/proxy.o
endif

ifeq ($(CONFIG_BT_MESH_CFG_CLI), y)
ZBLUE_OBJS += host/cfg_cli.o
endif

ifeq ($(CONFIG_BT_MESH_HEALTH_CLI), y)
ZBLUE_OBJS += host/health_cli.o
endif

ifeq ($(CONFIG_BT_MESH_SELF_TEST), y)
ZBLUE_OBJS += host/test.o
endif

ifeq ($(CONFIG_BT_MESH_CDB), y)
ZBLUE_OBJS += host/cdb.o
endif

ZBLUE_OBJS += port/hex.o
ZBLUE_OBJS += port/buf.o
ZBLUE_OBJS += port/poll.o
ZBLUE_OBJS += port/work_q.o
ZBLUE_OBJS += port/porting.o
ZBLUE_OBJS += port/queue.o
ZBLUE_OBJS += port/mem_slab.o
ZBLUE_OBJS += port/dec.o

ifneq ($(findstring $(CONFIG_BT_XR829)$(CONFIG_BT_XR819S), y), )
ZBLUE_OBJS += port/xradio.o
ZBLUE_OBJS += port/platform_bsp.o
endif

ifeq ($(CONFIG_BTSNOOP), y)
ZBLUE_OBJS += port/btsnoop.o
ZBLUE_OBJS += port/save_log_by_uart.o
endif

ifeq ($(CONFIG_BT_SETTINGS), y)
ZBLUE_OBJS += settings/source/settings.o
#CONFIG_SETTINGS_FS
ZBLUE_OBJS += settings/source/settings_file.o
ZBLUE_OBJS += settings/source/settings_init.o
ZBLUE_OBJS += settings/source/settings_store.o
ZBLUE_OBJS += settings/source/settings_line.o
ZBLUE_OBJS += settings/source/settings_runtime.o
endif

ZBLUE += $(ZBLUE_OBJS)

CFLAGS +=-DCONFIG_ARM -DZTEST_UNITTEST -D$(HCIOPTION)

ifeq ($(HCIOPTION), H4)
CFLAGS +=-DCONFIG_UART_0_INTERRUPT_DRIVEN -DCONFIG_UART_INTERRUPT_DRIVEN -DCONFIG_BT_H4
endif

CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/port
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/port/include
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/port/include/drivers
CFLAGS += -I components/thirdparty/tinycrypt/include
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/settings
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/common
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/host
CFLAGS += -I components/common/thirdparty/bluetooth/zblue

CFLAGS += -I include/drivers/
CFLAGS += -I include/freertos/
CFLAGS += -I include
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/include
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/include/bluetooth
CFLAGS += -I components/common/thirdparty/bluetooth/zblue/include/bluetooth/drivers

obj-y += $(ZBLUE)

endif
