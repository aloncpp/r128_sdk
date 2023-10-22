################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_TC = \
cmac_mode.c \
aes_encrypt.c \
utils.c \
sha256.c \
hmac.c \
hmac_prng.c \
ecc.c \
ecc_dh.c

C_SRCS_TC_NORMAL = $(filter-out $(INSTRUMENTATION_LIST),$(C_SRCS_TC))
C_SRCS_TC_INSTRU = $(filter $(INSTRUMENTATION_LIST),$(C_SRCS_TC))
C_NAME_TC_NORMAL = $(C_SRCS_TC_NORMAL:%.c=%)
C_NAME_TC_INSTRU = $(C_SRCS_TC_INSTRU:%.c=%)

OBJS_TC = $(C_SRCS_TC:%.c=$(ROOT_PATH)/src/ble/build/tinycrypt/source/%.o)

C_DEPS_TC = $(C_SRCS_TC:%.c=$(ROOT_PATH)/src/ble/build/tinycrypt/source/%.d)

C_NAME_TC = $(C_SRCS_TC:%.c=%)

C_INCLUDE += \
-I$(ROOT_PATH)/src/ble \
-I$(ROOT_PATH)/include/ble \
-I$(ROOT_PATH)/include/ble/drivers \
-I$(ROOT_PATH)/src/ble/common/ \
-I$(ROOT_PATH)/src/ble/tinycrypt/include

# Each subdirectory must supply rules for building sources it contributes

$(foreach i, $(C_NAME_TC_NORMAL),$(eval $(call one_compile,tinycrypt/source,$(i),)))
$(foreach i, $(C_NAME_TC_INSTRU),$(eval $(call one_compile,tinycrypt/source,$(i),-finstrument-functions)))
