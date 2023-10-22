#
# Copyright (c) 2001, 2002 Swedish Institute of Computer Science.
# All rights reserved. 
# 
# Redistribution and use in source and binary forms, with or without modification, 
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission. 
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
# SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
# OF SUCH DAMAGE.
#
# This file is part of the lwIP TCP/IP stack.
# 
# Author: Adam Dunkels <adam@sics.se>
#

# COREFILES, CORE4FILES: The minimum set of files needed for lwIP.

obj-y += core/init.o \
	 core/def.o \
	 core/dns.o \
	 core/inet_chksum.o \
	 core/ip.o \
	 core/mem.o \
	 core/memp.o \
	 core/netif.o \
	 core/pbuf.o \
	 core/raw.o \
	 core/sys.o \
	 core/tcp.o \
	 core/tcp_in.o \
	 core/tcp_out.o \
	 core/timeouts.o \
	 core/udp.o

obj-y += core/ipv4/autoip.o \
	 core/ipv4/dhcp.o \
	 core/ipv4/etharp.o \
	 core/ipv4/icmp.o \
	 core/ipv4/igmp.o \
	 core/ipv4/ip4_frag.o \
	 core/ipv4/ip4.o \
	 core/ipv4/ip4_addr.o

obj-y += core/ipv6/dhcp6.o \
	 core/ipv6/ethip6.o \
	 core/ipv6/icmp6.o \
	 core/ipv6/inet6.o \
	 core/ipv6/ip6.o \
	 core/ipv6/ip6_addr.o \
	 core/ipv6/ip6_frag.o \
	 core/ipv6/mld6.o \
	 core/ipv6/nd6.o

obj-y += arch/sys_arch.o \
		   arch/checksum.o \
		   arch/memcpy.o
		  # /arch/net_debug.o \
		  # /arch/aw_stats.o \
		  # /arch/tftp_arch.o

# APIFILES: The files which implement the sequential and socket APIs.
obj-y += api/api_lib.o \
	 api/api_msg.o \
	 api/err.o \
	 api/netbuf.o \
	 api/netdb.o \
	 api/netifapi.o \
	 api/sockets.o \
	 api/tcpip.o

# NETIFFILES: Files implementing various generic network interface functions
obj-y += netif/ethernet.o \
	 netif/slipif.o

# SIXLOWPAN: 6LoWPAN
obj-y += netif/lowpan6.o \

# PPPFILES: PPP
PPPFILES= netif/ppp/auth.o \
	 netif/ppp/ccp.o \
	 netif/ppp/chap-md5.o \
	 netif/ppp/chap_ms.o \
	 netif/ppp/chap-new.o \
	 netif/ppp/demand.o \
	 netif/ppp/eap.o \
	 netif/ppp/ecp.o \
	 netif/ppp/eui64.o \
	 netif/ppp/fsm.o \
	 netif/ppp/ipcp.o \
	 netif/ppp/ipv6cp.o \
	 netif/ppp/lcp.o \
	 netif/ppp/magic.o \
	 netif/ppp/mppe.o \
	 netif/ppp/multilink.o \
	 netif/ppp/ppp.o \
	 netif/ppp/pppapi.o \
	 netif/ppp/pppcrypt.o \
	 netif/ppp/pppoe.o \
	 netif/ppp/pppol2tp.o \
	 netif/ppp/pppos.o \
	 netif/ppp/upap.o \
	 netif/ppp/utils.o \
	 netif/ppp/vj.o \
	 netif/ppp/polarssl/arc4.o \
	 netif/ppp/polarssl/des.o \
	 netif/ppp/polarssl/md4.o \
	 netif/ppp/polarssl/md5.o \
	 netif/ppp/polarssl/sha1.o


# SNMPFILES: SNMPv2c agent
#obj-y += apps/snmp/snmp_asn1.o \
	 apps/snmp/snmp_core.o \
	 apps/snmp/snmp_mib2.o \
	 apps/snmp/snmp_mib2_icmp.o \
	 apps/snmp/snmp_mib2_interfaces.o \
	 apps/snmp/snmp_mib2_ip.o \
	 apps/snmp/snmp_mib2_snmp.o \
	 apps/snmp/snmp_mib2_system.o \
	 apps/snmp/snmp_mib2_tcp.o \
	 apps/snmp/snmp_mib2_udp.o \
	 apps/snmp/snmp_msg.o \
	 apps/snmp/snmpv3.o \
	 apps/snmp/snmp_netconn.o \
	 apps/snmp/snmp_pbuf_stream.o \
	 apps/snmp/snmp_raw.o \
	 apps/snmp/snmp_scalar.o \
	 apps/snmp/snmp_table.o \
	 apps/snmp/snmp_threadsync.o \
	 apps/snmp/snmp_traps.o \
	 apps/snmp/snmpv3_mbedtls.o \
	 apps/snmp/snmpv3_dummy.o

# HTTPDFILES: HTTP server
#obj-y += apps/httpd/fs.o \
	 apps/httpd/httpd.o

# LWIPERFFILES: IPERF server
#obj-y += apps/lwiperf/lwiperf.o

# SNTPFILES: SNTP client
#obj-y += apps/sntp/sntp.o

# MDNSFILES: MDNS responder
#obj-y += apps/mdns/mdns.o

# NETBIOSNSFILES: NetBIOS name server
#obj-y += apps/netbiosns/netbiosns.o

# TFTPFILES: TFTP server files
#obj-y += apps/tftp/tftp_server.o

# MQTTFILES: MQTT client files
#obj-y += apps/mqtt/mqtt.o

include components/common/aw/xradio/plt_def.mk
CFLAGS += $(PLT_CFLAGS)

CFLAGS += -Iinclude/drivers/
CFLAGS += -Iinclude/freertos/

CFLAGS += -Icomponents/common/thirdparty/network/lwip-2.1.2/src/include

ifeq ($(CONFIG_ARCH_SUN8IW18P1), y)
CFLAGS += -DLWIP_FREERTOS_ORIG=0
else ifeq ($(CONFIG_ARCH_SUN20IW2P1), y)
CFLAGS += -DLWIP_FREERTOS_ORIG=1
CFLAGS += -Icomponents/common/aw/xradio/include/libc
endif


