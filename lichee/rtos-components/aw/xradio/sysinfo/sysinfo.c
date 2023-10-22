/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef CONFIG_COMPONENTS_LWIP

#include <string.h>
#include <stdlib.h>

#include "kernel/os/os.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "sysinfo/sysinfo.h"
#include "sysinfo_debug.h"
#ifdef CONFIG_DRIVERS_TRNG
#include "hal/sunxi_hal_trng.h"
#endif
#ifdef CONFIG_ARCH_SUN20IW2
#include "sunxi_hal_efuse.h"
#endif

static struct sysinfo g_sysinfo;

static uint8_t m_sysinfo_mac_addr[] = { 0x00, 0x80, 0xE1, 0x29, 0xE8, 0xD1 };
static uint8_t m_sysinfo_mac_set;

static void sysinfo_gen_mac_random(uint8_t mac_addr[6])
{
#if defined(CONFIG_DRIVERS_TRNG) && defined(CONFIG_ARCH_SUN20IW2)
	uint32_t rand[4];
	HAL_TRNG_Extract(0, rand);
	for (int i = 0; i < 6; i++) {
		mac_addr[i] = *((uint8_t *)rand + i);
	}
#else
	for (int i = 0; i < 4; i++) {
		mac_addr[i] = *((uint8_t *)m_sysinfo_mac_addr + i);
	}
	/* random the last two bytes */
	srand((unsigned int)XR_OS_GetTicks() >> 3);
	mac_addr[4] = rand() % 255 + 1;
	srand((unsigned int)XR_OS_GetTicks());
	mac_addr[5] = rand() % 255 + 1;
#endif

	mac_addr[0] &= 0xFC;
}

static void sysinfo_gen_mac_by_chipid(uint8_t mac_addr[6])
{
	int i;
#if defined(CONFIG_DRIVERS_EFUSE) && defined(CONFIG_ARCH_SUN20IW2)
	uint8_t chipid[16];

	hal_efuse_get_chipid(chipid);

	for (i = 0; i < 2; ++i) {
		mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 12];
	}
	for (i = 2; i < 6; ++i) {
		mac_addr[i] = chipid[i] ^ chipid[i + 6] ^ chipid[i + 10];
	}
	mac_addr[0] &= 0xFC;
#else
	memset(mac_addr, 0, 6);
#endif
}

static void sysinfo_init_mac_addr(void)
{
#if PRJCONF_MAC_ADDR_FROM_APP_FIRST
	if (m_sysinfo_mac_set) {
		SYSINFO_DBG("get mac addr from app!\n");
		memcpy(g_sysinfo.mac_addr, m_sysinfo_mac_addr, SYSINFO_MAC_ADDR_LEN);
		return;
	}
#endif
	SYSINFO_DBG("internal mac addr source: %d\n", PRJCONF_MAC_ADDR_SOURCE);

	switch (PRJCONF_MAC_ADDR_SOURCE) {
	case SYSINFO_MAC_ADDR_CODE:
		memcpy(g_sysinfo.mac_addr, m_sysinfo_mac_addr, SYSINFO_MAC_ADDR_LEN);
		return;
	case SYSINFO_MAC_ADDR_EFUSE:
		//todo: support efuse read mac
		if (1) {
			SYSINFO_WRN("read mac addr from eFuse fail\n");
			goto random_mac_addr;
		}
		return;
	case SYSINFO_MAC_ADDR_CHIPID:
		sysinfo_gen_mac_by_chipid(g_sysinfo.mac_addr);
		for (int i = 0; i < sizeof(g_sysinfo.mac_addr); ++i) {
			if (g_sysinfo.mac_addr[i] != 0) {
				SYSINFO_DBG("mac addr gen by chipid!\n");
				return;
			}
		}
		goto random_mac_addr;
	case SYSINFO_MAC_ADDR_RANDOM:
		goto random_mac_addr;
	default:
		SYSINFO_WRN("invalid mac addr source\n");
		goto random_mac_addr;
	}

random_mac_addr:
	SYSINFO_DBG("use random mac addr\n");
	sysinfo_gen_mac_random(g_sysinfo.mac_addr);
}

static void sysinfo_init_value(void)
{
	sysinfo_default();
}

void sysinfo_set_default_mac(const uint8_t *macaddr)
{
	memcpy(m_sysinfo_mac_addr, macaddr, SYSINFO_MAC_ADDR_LEN);
	/* ensure MAC addr is valid, bit[48:47] must be '0x0' */
	m_sysinfo_mac_addr[0] &= 0xFC;
	m_sysinfo_mac_set = 1;
}

/**
 * @brief Initialize the sysinfo module
 * @return 0 on success, -1 on failure
 */
int sysinfo_init(void)
{
	sysinfo_init_value();
	return 0;
}

/**
 * @brief DeInitialize the sysinfo module
 * @return None
 */
void sysinfo_deinit(void)
{
}

/**
 * @brief Set default value to sysinfo
 * @return 0 on success, -1 on failure
 */
int sysinfo_default(void)
{
	memset(&g_sysinfo, 0, SYSINFO_SIZE);

	/* MAC address */
	sysinfo_init_mac_addr();

	/* wlan mode */
	g_sysinfo.wlan_mode = WLAN_MODE_STA;

	/* netif STA */
	g_sysinfo.sta_use_dhcp = 1;

	/* netif AP */
	IP4_ADDR(&g_sysinfo.netif_ap_param.ip_addr, 192, 168, 51, 1);
	IP4_ADDR(&g_sysinfo.netif_ap_param.net_mask, 255, 255, 255, 0);
	IP4_ADDR(&g_sysinfo.netif_ap_param.gateway, 192, 168, 51, 1);

	SYSINFO_DBG("set default value\n");
	return 0;
}

/**
 * @brief Get the pointer of the sysinfo
 * @return Pointer to the sysinfo, NULL on failure
 */
struct sysinfo *sysinfo_get(void)
{
	return &g_sysinfo;
}

#endif /* CONFIG_COMPONENTS_LWIP */
