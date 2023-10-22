obj-y += init-sun20iw2.o
obj-y += mmu.o
obj-y += cpufreq.o

ifeq ($(CONFIG_COMPONENTS_AW_AUDIO_SYSTEM),y)
CFLAGS +=  -I components/common/aw/AudioSystem/include
endif

ifeq ($(CONFIG_COMPONENTS_PM), y)
CFLAGS += -Icomponents/common/aw/pm/include/
endif

ifeq ($(CONFIG_COMPONENTS_XTENSA_HIFI5_NNLIB_LIBRARY), y)
CFLAGS +=  -I components/thirdparty/xtensa/hifi5_nn_v170_library/include
endif

ifeq ($(CONFIG_COMPONENTS_XTENSA_HIFI5_VFPU_LIBRARY), y)
CFLAGS +=  -I components/thirdparty/xtensa/hifi5_vfpu_v200_library/include
endif

ifeq ($(CONFIG_COMPONENTS_OMX_SYSTEM),y)
CFLAGS +=  -I components/common/aw/OpenMax/utils/common
endif
