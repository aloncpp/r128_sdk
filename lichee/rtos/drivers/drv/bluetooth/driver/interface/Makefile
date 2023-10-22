#hci
ifeq ($(CONFIG_BT_VHCI), y)
BT_INTERFACE_SELECT := vhci
else ifeq ($(CONFIG_BT_UART), y)
BT_INTERFACE_SELECT := uart
else
BT_INTERFACE_SELECT := vhci
endif

include $(BT_DRV_BASE)/driver/interface/$(BT_INTERFACE_SELECT)/Makefile
