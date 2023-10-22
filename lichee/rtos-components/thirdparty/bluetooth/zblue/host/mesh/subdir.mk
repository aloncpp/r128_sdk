################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS_MESH += \
main.c \
access.c \
adv.c \
beacon.c \
cfg_srv.c \
cfg_cli.c \
crypto.c \
friend.c \
health_srv.c \
health_cli.c \
lpn.c \
net.c \
prov.c \
proxy.c \
transport.c

OBJS_MESH += $(C_SRCS_MESH:%.c=$(ROOT_PATH)/src/ble/build/host/mesh/%.o)
#OBJS_MESH += $(ROOT_PATH)/src/ble/build/host/mesh/tracing.o

C_DEPS_MESH += $(C_SRCS_MESH:%.c=$(ROOT_PATH)/src/ble/build/host/mesh/%.d)
#C_DEPS_MESH += $(ROOT_PATH)/src/ble/build/host/mesh/tracing.d

C_NAME_MESH = $(C_SRCS_MESH:%.c=%)
	
# Each subdirectory must supply rules for building sources it contributes

$(foreach i, $(C_NAME_MESH),$(eval $(call one_compile,host/mesh,$(i))))
#$(ROOT_PATH)/src/ble/build/host/mesh/tracing.o : $(ROOT_PATH)/src/ble/boards/posix/native_posix/tracing.c
#	@echo 'Building file: $<'
#	@echo 'Invoking: GCC C Compiler'
#	$(CC) -DUSE_STDIO=1 -D__GCC_POSIX__=1 $(C_INCLUDE) $(CFLAGS) -fPIC -Wall -c -fmessage-length=0 -lpthread -lrt -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
#	@echo 'Finished building: $<'
#	@echo ' '

