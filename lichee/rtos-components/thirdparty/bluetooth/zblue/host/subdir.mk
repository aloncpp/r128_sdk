################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_HOST = \
uuid.c \
hci_core.c \
conn.c \
l2cap.c \
att.c \
gatt.c \
crypto.c \
smp.c \
keys.c \
hci_ecc.c \

C_SRCS_HOST_NORMAL = $(filter-out $(INSTRUMENTATION_LIST),$(C_SRCS_HOST))
C_SRCS_HOST_INSTRU = $(filter $(INSTRUMENTATION_LIST),$(C_SRCS_HOST))
C_NAME_HOST_NORMAL = $(C_SRCS_HOST_NORMAL:%.c=%)
C_NAME_HOST_INSTRU = $(C_SRCS_HOST_INSTRU:%.c=%)

OBJS_HOST = $(C_SRCS_HOST:%.c=$(ROOT_PATH)/src/ble/build/host/%.o)

C_DEPS_HOST = $(C_SRCS_HOST:%.c=$(ROOT_PATH)/src/ble/build/host/%.d)

C_NAME_HOST = $(C_SRCS_HOST:%.c=%)


ifeq ($(HCIOPTION),H4) 
	OBJS_HOST += $(ROOT_PATH)/src/ble/build/host/h4.o $(ROOT_PATH)/src/ble/build/host/device.o $(ROOT_PATH)/src/ble/build/host/uart_wrapper.o
	C_DEPS_HOST += $(ROOT_PATH)/src/ble/build/host/h4.d $(ROOT_PATH)/src/ble/build/host/device.d $(ROOT_PATH)/src/ble/build/host/uart_wrapper.d
	CFLAGS += -DCONFIG_BT_H4 -DCONFIG_ARM
endif

ifeq ($(HCIOPTION),VIRTUAL_UART) 
	OBJS_HOST    += $(ROOT_PATH)/src/ble/build/host/h4.o $(ROOT_PATH)/src/ble/build/host/device.o $(ROOT_PATH)/src/ble/build/host/uart_wrapper.o $(ROOT_PATH)/src/ble/build/host/virtual_hci.o
	C_DEPS_HOST  += $(ROOT_PATH)/src/ble/build/host/h4.d $(ROOT_PATH)/src/ble/build/host/device.d $(ROOT_PATH)/src/ble/build/host/uart_wrapper.d $(ROOT_PATH)/src/ble/build/host/virtual_hci.d
	CFLAGS += -DCONFIG_BT_H4 -DCONFIG_ARM -DVIRTUAL_UART
endif

ifeq ($(HCIOPTION),VIRTUAL_HCI) 
	OBJS_HOST    +=  $(ROOT_PATH)/src/ble/build/host/virtual_hci.o
	C_DEPS_HOST  +=  $(ROOT_PATH)/src/ble/build/host/virtual_hci.d
	CFLAGS += -DCONFIG_BT_H4 -DCONFIG_ARM -DVIRTUAL_HCI
endif

#ifeq ($(HCIOPTION),H4) 
#	C_SRCS += ../bluetooth/driver/bluetooth/serial/uart_mcux_lpuart.c
#	OBJS += ./bluetooth/driver/bluetooth/serial/uart_mcux_lpuart.o
#	C_DEPS += ./bluetooth/driver/bluetooth/serial/uart_mcux_lpuart.d
#endif

C_INCLUDE += \
-I$(ROOT_PATH)/src/ble/common \
-I$(ROOT_PATH)/src/ble/boards/posix/native_posix \
-I$(ROOT_PATH)/src/ble/arch/posix/include \
-I$(ROOT_PATH)/src/ble/arch/posix/soc/inf_clock \
-I$(ROOT_PATH)/src/ble/tinycrypt/include \
-I$(ROOT_PATH)/src/ble/boards/$(BOARD) \
-I$(ROOT_PATH)/include \
-I$(ROOT_PATH)/include/ble \
-I$(ROOT_PATH)/include/driver \
-I$(ROOT_PATH)/include/driver/cmsis \
-I$(ROOT_PATH)/src/ble/controller/shrd_utils/src/porting/ \
-I$(ROOT_PATH)/src/ble/controller/shrd_utils/src/porting/$(BOARD)/ \
-I$(ROOT_PATH)/src/ble/controller/shrd_utils/src/porting/$(BOARD)/inc \
-I$(ROOT_PATH)/src/ble/controller/shrd_utils/src/porting/$(BOARD)/cm4 \
-I$(ROOT_PATH)/src/ble/controller/shrd_utils/inc/ \

# Each subdirectory must supply rules for building sources it contributes

$(foreach i, $(C_NAME_HOST_NORMAL),$(eval $(call one_compile,host,$(i),)))
$(foreach i, $(C_NAME_HOST_INSTRU),$(eval $(call one_compile,host,$(i),-finstrument-functions)))

