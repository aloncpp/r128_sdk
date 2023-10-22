##############################################################################
#
##############################################################################


##############################################################################
# B U I L D    E N V I R O N M E N T
##############################################################################

QUIET = @

##############################################################################
# Required definitions

#ROOT_PATH = ../..
ROOT_PATH = $(BASE)/components/common/thirdparty/bluetooth/zephyr

#include $(ROOT_PATH)/gcc.mk

HCIOPTION = VIRTUAL_HCI
USE_BLE_HOST_MESH = n

OBJS_BLE_HOST = $(OBJS_COMMON) $(OBJS_HOST) $(OBJS_TC)

ifeq ($(USE_BLE_HOST_MESH), y)
include $(ROOT_PATH)/src/mesh/subdir.mk
OBJS_BLE_HOST += $(OBJS_MESH)
endif

INCLUDE_PATHS += -I$(ROOT_PATH)/include/ble \
-I$(ROOT_PATH)/src/tinycrypt/include \
-I$(ROOT_PATH)/src \
-I$(ROOT_PATH)/include/ble/drivers \
-I$(ROOT_PATH)/include/driver/chip \
-I$(ROOT_PATH)/include/ble/sys \
-I$(ROOT_PATH)/include

ifeq ($(CONFIG_BT_SETTINGS), y)
INCLUDE_PATHS += -I$(ROOT_PATH)/src/settings
endif

CC_SYMBOLS += -DCONFIG_ARM -DZTEST_UNITTEST -D$(HCIOPTION)
ifeq ($(HCIOPTION), H4)
CC_SYMBOLS += -DCONFIG_UART_0_INTERRUPT_DRIVEN -DCONFIG_UART_INTERRUPT_DRIVEN -DCONFIG_BT_H4
endif

#LIBS := lib_ble_host.a

#ifeq ($(USE_BLE_HOST_MESH), y)
XRADIO_ZEPHYR_DIRS := common tinycrypt/source porting net
#else
#XRADIO_DIRS := ./common ./host ./tinycrypt/source/
#endif
ifeq ($(CONFIG_BT_SETTINGS), y)
XRADIO_ZEPHYR_DIRS += settings/source
endif

$(info ****************************)
$(info ******* XRADIO_DIRS=$(XRADIO_ZEPHYR_DIRS) *******)
$(info ****************************)

ifeq ($(CONFIG_BT_HCI_RAW), y)
  BASE_SRCS += host/hci_raw.c
endif

ifeq ($(CONFIG_BT_TINYCRYPT_ECC), y)
  BASE_SRCS += host/hci_ecc.c
endif

ifeq ($(CONFIG_BT_SETTINGS), y)
  BASE_SRCS += host/settings.c
endif

ifeq ($(CONFIG_BT_HOST_CCM), y)
  BASE_SRCS += host/aes_ccm.c
endif

ifeq ($(CONFIG_BT_HCI_HOST), y)
  BASE_SRCS += host/uuid.c host/addr.c host/buf.c host/hci_core.c host/hci_common.c

  ifeq ($(CONFIG_BT_HOST_CRYPTO), y)
    BASE_SRCS += host/crypto.c
  endif

  ifeq ($(CONFIG_BT_ECC), y)
    BASE_SRCS += host/ecc.c
  endif

  ifeq ($(CONFIG_BT_CONN), y)
    BASE_SRCS += host/conn.c host/l2cap.c host/att.c host/gatt.c

    ifeq ($(CONFIG_BT_SMP), y)
      BASE_SRCS += host/smp.c host/keys.c
    else
      BASE_SRCS += host/smp_null.c
    endif
  endif

  ifeq ($(CONFIG_BT_ISO), y)
    BASE_SRCS += host/iso.c
  endif

  ifeq ($(CONFIG_BT_DF), y)
    BASE_SRCS += host/direction.c
  endif
endif

ifeq ($(CONFIG_BT_MESH), y)
MESH_SRCS += \
  mesh/main.c \
  mesh/cfg.c \
  mesh/adv.c \
  mesh/beacon.c \
  mesh/net.c \
  mesh/subnet.c \
  mesh/app_keys.c \
  mesh/transport.c \
  mesh/rpl.c \
  mesh/heartbeat.c \
  mesh/crypto.c \
  mesh/access.c \
  mesh/cfg_srv.c \
  mesh/health_srv.c \
  mesh/generic_onoff_cli.c \
  mesh/generic_onoff_srv.c \
  mesh/mesh_common.c \
  mesh/transition.c

ifeq ($(CONFIG_BT_SETTINGS), y)
  MESH_SRCS += mesh/settings.c
endif
endif

ifeq ($(CONFIG_BT_MESH_ADV_LEGACY), y)
  MESH_SRCS += mesh/adv_legacy.c
endif

ifeq ($(CONFIG_BT_MESH_ADV_EXT), y)
  MESH_SRCS += mesh/adv_ext.c
endif

ifeq ($(CONFIG_BT_MESH_LOW_POWER), y)
  MESH_SRCS += mesh/lpn.c
endif

ifeq ($(CONFIG_BT_MESH_FRIEND), y)
  MESH_SRCS += mesh/friend.c
endif

ifeq ($(CONFIG_BT_MESH_PROV), y)
  MESH_SRCS += mesh/prov.c
endif

ifeq ($(CONFIG_BT_MESH_PROV_DEVICE), y)
  MESH_SRCS += mesh/prov_device.c
endif

ifeq ($(CONFIG_BT_MESH_PROVISIONER), y)
  MESH_SRCS += mesh/provisioner.c
endif

ifeq ($(CONFIG_BT_MESH_PB_ADV), y)
  MESH_SRCS += mesh/pb_adv.c
endif

ifeq ($(CONFIG_BT_MESH_PB_GATT), y)
  MESH_SRCS += mesh/pb_gatt.c
endif

ifeq ($(CONFIG_BT_MESH_PROXY), y)
  MESH_SRCS += mesh/proxy.c
endif

ifeq ($(CONFIG_BT_MESH_CFG_CLI), y)
  MESH_SRCS += mesh/cfg_cli.c
endif

ifeq ($(CONFIG_BT_MESH_HEALTH_CLI), y)
  MESH_SRCS += mesh/health_cli.c
endif

ifeq ($(CONFIG_BT_MESH_SELF_TEST), y)
  MESH_SRCS += mesh/test.c
endif

#ifeq ($(CONFIG_BT_MESH_SHELL), y)
  #MESH_SRCS += mesh/shell.c
#endif

ifeq ($(CONFIG_BT_MESH_CDB), y)
  MESH_SRCS += mesh/cdb.c
endif

XRADIO_ZEPHYR_SRCS := $(subst $(BASE)/components/common/thirdparty/bluetooth/zephyr/, , $(basename $(addprefix $(BASE)/components/common/thirdparty/bluetooth/zephyr/src/,$(BASE_SRCS) $(MESH_SRCS)) $(foreach dir,$(addprefix $(BASE)/components/common/thirdparty/bluetooth/zephyr/src/,$(XRADIO_ZEPHYR_DIRS)),$(wildcard $(dir)/*.[csS]))))
XRADIO_ZEPHYR_OBJS += $(addsuffix .o,$(XRADIO_ZEPHYR_SRCS))

# library make rules
#include $(LIB_MAKE_RULES)
