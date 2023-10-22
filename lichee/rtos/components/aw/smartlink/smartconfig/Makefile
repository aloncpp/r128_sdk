#
# Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the
#       distribution.
#    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
#       its contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
obj-y += src/smart_config_ack.o \
                                 src/smart_config_crc.o \
                                 src/smart_config_decode.o \
                 	             src/smart_config_main.o \
                	             examples/cmd_smart_config.o \
                                 examples/cmd_smart_config_entry.o

CFLAGS += -I components/aw/smartlink/smartconfig/include
CFLAGS += -I components/aw/smartlink/smartconfig/examples

CFLAGS += -I components/common/aw/xradio/cmd

#######lwip########
ifeq ($(CONFIG_COMPONENTS_LWIP_2_1_2),y)
LWIP = lwip-2.1.2
else ifeq ($(CONFIG_COMPONENTS_LWIP_2_0_3),y)
LWIP = lwip-2.0.3
endif
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include/lwip
CFLAGS += -I components/common/thirdparty/network/$(LWIP)/src/include
##################

ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_7_0),y)
MBEDTLS = mbedtls-2.7.0
else ifeq ($(CONFIG_COMPONENTS_MBEDTLS_2_16_0),y)
MBEDTLS = mbedtls-2.16.0
endif
CFLAGS += -I components/common/thirdparty/network/$(MBEDTLS)/include

CFLAGS += -I drivers/drv/wireless/xradio/os
CFLAGS += -I drivers/drv/wireless/xradio/os/include
CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl
CFLAGS += -I drivers/drv/wireless/xradio/adapter/net_ctrl/sys_ctrl
CFLAGS += -I drivers/drv/wireless/xradio/adapter
CFLAGS += -I drivers/drv/wireless/xradio/include/smartlink
CFLAGS += -I drivers/drv/wireless/xradio/include

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)
