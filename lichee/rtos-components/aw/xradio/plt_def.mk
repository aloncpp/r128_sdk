XRADIO_SRCDIR = $(BUILD_DIR)/components/common/aw/xradio

ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
PLT_CFLAGS += -I $(BASE)/tools/gcc-arm-melis-eabi-8-2019-q3-update/arm-melis-eabi/include
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/include/
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/port/
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS/Source/portable/GCC/ARM_CA9
else ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
ifeq ($(CONFIG_ARCH_RISCV)_$(CONFIG_ARCH_RISCV_C906), y_y)
PLT_CFLAGS += -I $(BASE)/tools/riscv64-elf-x86_64-20201104/include
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/include/
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/port/
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/portable/GCC/RISC-V/
PLT_CFLAGS += -I $(BASE)/include/arch/riscv/
PLT_CFLAGS += -I $(BASE)/arch/risc-v/
else ifeq ($(CONFIG_ARCH_ARM_CORTEX_M33), y)
PLT_CFLAGS += -I $(BASE)/tools/gcc-arm-none-eabi-8-2019-q3-update/arm-none-eabi/include
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/include/
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/port/
PLT_CFLAGS += -I $(BASE)/kernel/FreeRTOS-orig/Source/portable/GCC/ARM_CM33_NTZ/non_secure/
PLT_CFLAGS += -I $(BASE)/arch/arm/armv8m/include/cmsis/
endif
endif

PLT_CFLAGS += -I $(BASE)/components/common/aw/xradio/include/
