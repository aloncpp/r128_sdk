################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_SETTINGS = \
settings.c \
settings_file.c \
settings_init.c \
settings_store.c \
settings_line.c \
settings_priv.h \
settings_runtime.c

C_SRCS_SETTINGS_NORMAL = $(filter-out $(INSTRUMENTATION_LIST),$(C_SRCS_SETTINGS))
C_SRCS_SETTINGS_INSTRU = $(filter $(INSTRUMENTATION_LIST),$(C_SRCS_SETTINGS))
C_NAME_SETTINGS_NORMAL = $(C_SRCS_SETTINGS_NORMAL:%.c=%)
C_NAME_SETTINGS_INSTRU = $(C_SRCS_SETTINGS_INSTRU:%.c=%)

OBJS_SETTINGS = $(C_SRCS_SETTINGS:%.c=$(ROOT_PATH)/src/ble/build/SETTINGS/%.o)

C_DEPS_SETTINGS = $(C_SRCS_SETTINGS:%.c=$(ROOT_PATH)/src/ble/build/SETTINGS/%.d)

C_NAME_SETTINGS = $(C_SRCS_SETTINGS:%.c=%)


C_INCLUDE += \
-I$(ROOT_PATH)/src/ble \
-I$(ROOT_PATH)/include/ble \
-I$(ROOT_PATH)/include/kernel/FreeRTOS

# Each subdirectory must supply rules for building sources it contributes

$(foreach i, $(C_NAME_SETTINGS_NORMAL),$(eval $(call one_compile,SETTINGS,$(i), )))
$(foreach i, $(C_NAME_SETTINGS_INSTRU),$(eval $(call one_compile,SETTINGS,$(i),-finstrument-functions)))

