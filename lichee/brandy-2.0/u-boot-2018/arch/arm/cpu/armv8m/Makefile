# SPDX-License-Identifier: GPL-2.0+
#
# (C) Copyright 2000-2006
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.

extra-y := start.o

ifeq ($(CONFIG_ARCH_SUNXI),y)
extra-y	+= spare_head.o
endif

ifneq ($(CONFIG_SKIP_LOWLEVEL_INIT),y)
obj-y	+= lowlevel_init.o
endif

obj-$(CONFIG_SYS_ARCH_TIMER) += systick-timer.o
obj-y += cpu.o cache.o

obj-y += nvic/
#obj-$(CONFIG_ARCH_SUNXI) += nvic/
#obj-$(CONFIG_ARCH_SUNXI) += sunxi/
