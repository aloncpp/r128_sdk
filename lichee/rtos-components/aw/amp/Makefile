CFLAGS += -Icomponents/common/aw/amp/
CFLAGS += -Icomponents/common/aw/amp/service/demo/
CFLAGS += -Idrivers/rtos-hal/hal/source/msgbox/

CFLAGS += -Icomponents/common/aw/amp/service/pm/
CFLAGS += -Icomponents/common/aw/pm/include/
obj-y += amp_core.o
obj-y += amp_service.o
obj-y += amp_stub.o
obj-y += amp_msgbox.o
obj-y += amp_test.o
obj-y += msgbuffer.o

obj-$(CONFIG_AMP_FUNCCALL_THREADPOOL) += amp_threadpool.o

obj-$(CONFIG_AMP_TFM_STUB) += service/tfm/tfm_stub.o
obj-$(CONFIG_AMP_TFM_SERVICE) += service/tfm/tfm_ser.o

obj-$(CONFIG_AMP_FSYS_STUB) += service/fsys/fsys_stub.o
obj-$(CONFIG_AMP_FSYS_SERVICE) += service/fsys/fsys_ser.o

obj-$(CONFIG_AMP_NET_STUB) += service/net/network_stub.o
obj-$(CONFIG_AMP_NET_SERVICE) += service/net/network_ser.o

ifeq ($(CONFIG_DRIVERS_XRADIO)_$(CONFIG_DRIVER_R128),y_y)
LWIP = lwip-2.1.2
ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3), y)
LWIP = lwip-2.0.3
endif

CFLAGS += -Icomponents/common/aw/xradio/include/
CFLAGS += -Icomponents/common/thirdparty/network/$(LWIP)/src/include/
CFLAGS += -Idrivers/drv/wireless/xradio/include/
endif

ifeq ($(CONFIG_XRADIO_BT_CONTROLLER), y)
BT_CONTROLLER_SELECT := xradio
ifeq ($(CONFIG_SUN20IW2_BT_CONTROLLER), y)
XRADIO_CONTROLLER_SELECT := $(BT_CONTROLLER_SELECT)/sun20iw2
else
XRADIO_CONTROLLER_SELECT := $(BT_CONTROLLER_SELECT)/sun20iw2
endif
else
BT_CONTROLLER_SELECT := xradio
endif

obj-$(CONFIG_AMP_BT_STUB) += service/bt/bt_stub.o
obj-$(CONFIG_AMP_BT_SERVICE) += service/bt/bt_ser.o
CFLAGS += -Icomponents/aw/bluetooth/xradio/manager/include
CFLAGS += -Idrivers/drv/bluetooth/driver/controller/$(XRADIO_CONTROLLER_SELECT)/hal/include
CFLAGS += -Idrivers/drv/bluetooth/driver/controller/$(XRADIO_CONTROLLER_SELECT)/pal/include

obj-$(CONFIG_AMP_DEMO_STUB) += service/demo/demo_stub.o
obj-$(CONFIG_AMP_DEMO_SERVICE) += service/demo/demo_service.o

obj-$(CONFIG_AMP_CONSOLE_STUB) += service/rpcconsole/rpcconsole_stub.o
obj-$(CONFIG_AMP_CONSOLE_SERVICE) += service/rpcconsole/rpcconsole_ser.o

obj-$(CONFIG_AMP_MISC_STUB) += service/misc/misc_stub.o
obj-$(CONFIG_AMP_MISC_SERVICE) += service/misc/misc_ser.o

obj-$(CONFIG_AMP_AUDIO_STUB) += service/audio/audio_stub.o
obj-$(CONFIG_AMP_AUDIO_SERVICE) += service/audio/audio_ser.o

ifeq ($(CONFIG_AMP_AUDIO_SERVICE),y)
CFLAGS +=  -I components/common/aw/AudioSystem/include
endif

ifeq ($(CONFIG_AMP_AUDIO_STUB),y)
CFLAGS +=  -I components/common/aw/AudioSystem/include
CFLAGS +=  -I components/common/aw/AudioSystem/utils
endif

obj-$(CONFIG_AMP_TESTS) += tests/test_stress.o
obj-$(CONFIG_AMP_RPDATA_STUB) += service/rpdata/rpdata_stub.o
obj-$(CONFIG_AMP_RPDATA_SERVICE) += service/rpdata/rpdata_ser.o

ifeq ($(CONFIG_COMPONENTS_PM), y)
CFLAGS += -Icomponents/common/aw/pm/include/
obj-$(CONFIG_AMP_PMOFM33_STUB) += service/pm/pmofm33_stub.o
obj-$(CONFIG_AMP_PMOFM33_SERVICE) += service/pm/pmofm33_ser.o

obj-$(CONFIG_AMP_PMOFRV_STUB) += service/pm/pmofrv_stub.o
obj-$(CONFIG_AMP_PMOFRV_SERVICE) += service/pm/pmofrv_ser.o

obj-$(CONFIG_AMP_PMOFDSP_STUB) += service/pm/pmofdsp_stub.o
obj-$(CONFIG_AMP_PMOFDSP_SERVICE) += service/pm/pmofdsp_ser.o

obj-$(CONFIG_AMP_FLASHC_STUB) += service/flashc/flashc_stub.o
obj-$(CONFIG_AMP_FLASHC_SERVICE) += service/flashc/flashc_ser.o

endif



