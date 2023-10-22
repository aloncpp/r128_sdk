#
#	Automated build configuration switches.
#

ifneq ($(PROJECT_CONFIG), y)
-include $(BASE)/.config
else
-include $(PROJECT_DIR)/.config
endif

ARCH		:=$(subst $(DB_QUOTES),,$(CONFIG_ARCH))
SUBARCH		:=$(subst $(DB_QUOTES),,$(CONFIG_SUBARCH))
TOOLCHAIN	:=$(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN))

include $(BASE)/kernel/objects.mk
include $(BASE)/drivers/objects.mk
include $(BASE)/arch/objects.mk
include $(BASE)/components/objects.mk
include $(BASE)/projects/objects.mk
include $(BASE)/.libs.mk

PYTHON:=$(CONFIG_DBUILD_PYTHON)

#GIT_DESCRIBE:=$(shell git --git-dir=$(BASE)/.git describe --dirty)

CC_MARCH 		:= $(subst $(DB_QUOTES),,$(CONFIG_ARCH_ARM_FAMILY))
CC_MTUNE		:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_CPU))
CC_TCFLAGS		:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_FLAGS))
CC_TCDEBUGFLAGS 	:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_DEBUG_FLAGS))
CC_OPTIMISE		:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_OPTIMISATION))
CC_WARNING		:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_WARNING))
CC_MACHFLAGS 		:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_MACH_FLAGS))
CC_MFPU			:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_MFPU))
CC_FPU_ABI		:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_FPU_ABI))
CC_OTHER_FLAGS:= $(subst $(DB_QUOTES),,$(CONFIG_TOOLCHAIN_OTHER_FLAGS))

ifeq ($(CONFIG_KASAN), y)
KASAN_SHADOW_OFFSET ?= $(CONFIG_KASAN_SHADOW_OFFSET)
ifeq ($(CONFIG_KASAN_INLINE), y)
	call_threshold := 10000
else
	call_threshold := 0
endif
CFLAGS_KASAN := -fsanitize=kernel-address \
		-fasan-shadow-offset=$(KASAN_SHADOW_OFFSET) \
		--param asan-stack=1 --param asan-globals=1 \
		--param asan-instrument-allocas=1 -fsanitize-address-use-after-scope \
		--param asan-instrumentation-with-call-threshold=$(call_threshold)
$(OBJECTS) $(OBJECTS-y): CFLAGS += $(CFLAGS_KASAN)
EXPORT_CFLAGS += $(CFLAGS_KASAN)
endif

ifeq ($(CONFIG_COMPONENTS_STACK_PROTECTOR), y)
$(OBJECTS) $(OBJECTS-y): CFLAGS += -fstack-protector-strong
$(OBJECTS) $(OBJECTS-y): CXXFLAGS += -fstack-protector-strong
LDFLAGS += -nostartfiles -nodefaultlibs
endif

INC_ROOTS  = $(BASE)/include/FreeRTOS_POSIX/ $(BASE)/include/drivers/
#INC_ROOTS += $(BASE)/include/bluetooth/ $(BASE)/include/cjson/ $(BASE)/drivers/rtos-hal/include/hal
INC_ROOTS += $(BASE)/include/cjson/ $(BASE)/drivers/rtos-hal/include/hal $(BASE)/drivers/rtos-hal/include/
INC_ROOTS += $(BASE)/drivers/rtos-hal/include/osal/ $(BASE)/include/sys/
INCDIRS   =  $(shell find $(INC_ROOTS) -maxdepth 6 -type d | grep -v '\.git')
INCLUDES += -I $(BASE)/kernel/Posix/include -I $(BASE)/kernel/Posix/include/portable
INCLUDES += -I $(BASE)/include/ $(foreach dir,$(INCDIRS),-I $(dir))
INCLUDES += -I $(BASE)/include/generated/$(RTOS_PROJECT_NAME)
INCLUDES += -I $(BASE)/kernel/
INCLUDES += -I $(BASE)/drivers/

ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
INCLUDES += -I $(BASE)/kernel/FreeRTOS/Source/include/ -I $(BASE)/kernel/FreeRTOS/Source/portable/GCC/ARM_CA9
INCLUDES += -I $(BASE)/include/arch/arm/armv7a/
INCLUDES += -I $(BASE)/include/arch/arm/mach/
endif

ifeq ($(CONFIG_ARCH_SUN8IW20P1), y)
INCLUDES += -I $(BASE)/kernel/FreeRTOS/Source/include/ -I $(BASE)/kernel/FreeRTOS/Source/portable/GCC/ARM_CA9
INCLUDES += -I $(BASE)/include/arch/arm/armv7a/
INCLUDES += -I $(BASE)/include/arch/arm/mach/
endif

ifeq ($(CONFIG_ARCH_SUN20IW2P1)_$(CONFIG_ARCH_ARM_CORTEX_M33), y_y)
INCLUDES += -I $(BASE)/kernel/FreeRTOS-orig/Source/include/ -I $(BASE)/kernel/FreeRTOS-orig/Source/portable/GCC/ARM_CM33_NTZ/non_secure/
INCLUDES += -I $(BASE)/include/arch/arm/armv8m/
INCLUDES += -I $(BASE)/include/arch/arm/mach/
endif
ifeq ($(CONFIG_ARCH_RISCV), y)
INCLUDES += -I $(BASE)/kernel/FreeRTOS-orig/Source/include/ -I $(BASE)/kernel/FreeRTOS-orig/Source/portable/GCC/RISC-V/
INCLUDES += -I $(BASE)/include/arch/riscv/
endif
INCLUDES += $(ARCH_INC)

ifeq ($(CONFIG_COMPONENT_CPLUSPLUS), y)
INCLUDES += -I $(BASE)/components/thirdparty/cplusplus
endif

CFLAGS_BASE=$(CC_OPTIMISE) $(CC_TCDEBUGFLAGS) $(CC_MARCH) $(CC_MFPU) $(CC_FPU_ABI) $(CC_MTUNE) $(CC_MACHFLAGS)
CFLAGS_BASE_1=-fno-builtin-printf -ffunction-sections -fdata-sections -fno-common -D_POSIX_MONOTONIC_CLOCK
CFLAGS_BASE_INC=-include $(BASE)/include/generated/$(RTOS_PROJECT_NAME)/autoconf.h -I $(BASE)/lib/include/ $(INCLUDES)
CFLAGS_BASE_INC += -I $(BASE)/projects/$(RTOS_TARGET_PROJECT_PATH)/src

ifeq ($(CONFIG_CHECK_ILLEGAL_FUNCTION_USAGE), y)
CFLAGS_BASE_1 += -finstrument-functions
endif

ifeq ($(CONFIG_ARCH_ARM), y)
CFLAGS_BASE_1 += -mno-unaligned-access
endif

EXPORT_CFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_1) $(CFLAGS_BASE_INC) $(CC_OTHER_FLAGS) -DCONFIG_OS_TINA
EXPORT_CXXFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_1) $(CFLAGS_BASE_INC) $(CC_OTHER_FLAGS)
EXPORT_ASFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_INC) -D__ASSEMBLY__ -DCONFIG_OS_TINA

$(OBJECTS) $(OBJECTS-y): CFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_1) $(CFLAGS_BASE_INC) $(CC_OTHER_FLAGS) -DCONFIG_OS_TINA
$(OBJECTS) $(OBJECTS-y): CXXFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_1) $(CFLAGS_BASE_INC) $(CC_OTHER_FLAGS)
$(OBJECTS) $(OBJECTS-y): CXXFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_1) $(CFLAGS_BASE_INC)
$(OBJECTS) $(OBJECTS-y): ASFLAGS += $(CFLAGS_BASE) $(CFLAGS_BASE_INC) -D__ASSEMBLY__ -DCONFIG_OS_TINA

EXPORT_CFLAGS += -include $(BASE)/kernel/Posix/include/FreeRTOS_POSIX.h
EXPORT_CXXFLAGS += -include $(BASE)/kernel/Posix/include/FreeRTOS_POSIX.h

$(OBJECTS) $(OBJECTS-y): CFLAGS += -include $(BASE)/kernel/Posix/include/FreeRTOS_POSIX.h
$(OBJECTS) $(OBJECTS-y): CXXFLAGS += -include $(BASE)/kernel/Posix/include/FreeRTOS_POSIX.h

ifeq ($(CONFIG_DEBUG_BACKTRACE_FRAME_POINTER), y)

EXPORT_CFLAGS += -mapcs-frame -fno-omit-frame-pointer -fno-optimize-sibling-calls
EXPORT_CXXFLAGS += -mapcs-frame -fno-omit-frame-pointer -fno-optimize-sibling-calls

$(OBJECTS) $(OBJECTS-y): CFLAGS += -mapcs-frame -fno-omit-frame-pointer -fno-optimize-sibling-calls
$(OBJECTS) $(OBJECTS-y): CXXFLAGS += -mapcs-frame -fno-omit-frame-pointer -fno-optimize-sibling-calls
endif

ifeq ($(PROJECT_CONFIG),y)
LINKER_SCRIPTS_CFLAGS += -I $(PROJECT_DIR)/include/
LINKER_SCRIPTS_CXXFLAGS += -I $(PROJECT_DIR)/include/
endif

LINKER_SCRIPTS_CFLAGS += -include $(BASE)/include/generated/$(RTOS_PROJECT_NAME)/autoconf.h
#
#	Link configuration.
#
ifeq ($(CONFIG_BUILD_NOSTDLIB), y)
LDFLAGS += -nostdlib
endif
ifeq ($(CONFIG_BUILD_GC_UNUSED), y)
LDFLAGS += -Wl,--gc-sections
endif

LDFLAGS += $(CFLAGS_BASE) -lm -lc -lgcc -lstdc++

ifeq ($(CONFIG_NEWLIBC_NANOSPECS), y)
LDFLAGS += --specs=nano.specs
ifeq ($(CONFIG_NEWLIBC_NANO_PRINTF_FLOAT), y)
LDFLAGS += -u _printf_float
endif
ifeq ($(CONFIG_NEWLIBC_NANO_SCANF_FLOAT), y)
LDFLAGS += -u _scanf_float
endif
endif

LDFLAGS += -Wl,--print-memory-usage

