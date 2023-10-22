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

#ifndef _NET_WLAN_WLAN_H_
#define _NET_WLAN_WLAN_H_

#include <stdint.h>
#ifndef CONFIG_OS_NUTTX
#include "lwip/netif.h"
#endif
#include "net/wlan/wlan_defs.h"
#include "net/ethernetif/ethernetif.h"

#ifdef __cplusplus
extern "C" {
#endif

/* wlan sys */
static __inline int wlan_sys_init(void)
{
	return 0;
}

static __inline int wlan_sys_deinit(void)
{
	return 0;
}

int wlan_attach(wlan_event_cb_func cb);
int wlan_detach(void);

static __inline struct netif *wlan_netif_create(enum wlan_mode mode)
{
	return ethernetif_create(mode);
}

static __inline int wlan_netif_delete(struct netif *nif)
{
	ethernetif_delete(nif);
	return 0;
}

static __inline enum wlan_mode wlan_if_get_mode(struct netif *nif)
{
	return ethernetif_get_mode(nif);
}

#if (defined(CONFIG_WLAN_STA) && defined(CONFIG_WLAN_AP))
int wlan_start(struct netif *nif);
int wlan_stop(struct netif *nif); /* Note: make sure wlan is disconnect before calling wlan_stop() */
#elif (defined(CONFIG_WLAN_STA))
int wlan_start_sta(struct netif *nif);
int wlan_stop_sta(struct netif *nif);
#define wlan_start(nif) wlan_start_sta(nif)
#define wlan_stop(nif)  wlan_stop_sta(nif)
#elif (defined(CONFIG_WLAN_AP))
int wlan_start_hostap(struct netif *nif);
int wlan_stop_hostap(struct netif *nif);
#define wlan_start(nif) wlan_start_hostap(nif)
#define wlan_stop(nif)  wlan_stop_hostap(nif)
#else
static __inline int wlan_start(struct netif *nif)
{
	return -1;
}
static __inline int wlan_stop(struct netif *nif)
{
	return -1;
}
#endif

int wlan_get_mac_addr(struct netif *nif, uint8_t *buf, int buf_len);
int wlan_set_mac_addr(struct netif *nif, uint8_t *mac_addr, int mac_len);
int wlan_set_ip_addr(struct netif *nif, uint8_t *ip_addr, int ip_len);

/**
 * @brief Set power save mode
 * @param[in] ifp Pointer to the network interface
 * @param[in] mode power save mode
 *     @arg 0 active mode
 *     @arg 1 power save mode
 * @return 0 on success
 */
int wlan_set_ps_mode(struct netif *nif, int mode);

/**
 * @brief Set application-specified IE to specified management frame
 * @param[in] nif Pointer to the network interface
 * @param[in] type Management frame type to be set
 *                 eg. (IEEE80211_FC_TYPE_MGMT | IEEE80211_FC_STYPE_BEACON)
 * @param[in] ie Pointer to application-specified IE data
 * @param[in] ie_len Length of application-specified IE data
 * @return 0 on success
 *
 * @note To delete existing application-specified IE, set ie to NULL, or set
 *       ie_len to 0.
 */
int wlan_set_appie(struct netif *nif, uint8_t type, uint8_t *ie, uint16_t ie_len);

/* STA */

/** @brief Flags for the input argument param of wlan_sta_config() */
#define WLAN_STA_CONF_FLAG_MFP  WPA_BIT(1)
#define WLAN_STA_CONF_FLAG_SAE  WPA_BIT(2)
#define WLAN_STA_CONF_FLAG_WPA3 (WLAN_STA_CONF_FLAG_MFP | WLAN_STA_CONF_FLAG_SAE)

/**
 * @brief Configure station in a convenient way to join a specified network
 * @param[in] ssid Network name, length is [1, 32]
 * @param[in] ssid_len The length of network name
 * @param[in] psk Network password, in one of the optional formats:
 *                - NULL or an empty string, to join an OPEN network
 *                - an ASCII string, length is [8, 63]
 *                - a hex string (two characters per octet of PSK), length is 64
 * @param[in] flag Flags of configuration:
 *                 - WLAN_STA_CONF_FLAG_MFP: support IEEE 802.11w (MFP)
 *                 - WLAN_STA_CONF_FLAG_SAE: support SAE
 *                 - WLAN_STA_CONF_FLAG_WPA3: support WPA3-PSK
 * @return 0 on success, -1 on failure
 *
 * @note This way is only adapted to join the OPEN, WPA/WPA2/WPA3-PSK network.
 */
int wlan_sta_config(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk, uint32_t flag);

/**
 * @brief Configure station in a convenient way to join a specified network
 * @param[in] ssid Network name, length is [1, 32]
 * @param[in] ssid_len The length of network name
 * @param[in] psk Network password, in one of the optional formats:
 *                - NULL or an empty string, to join an OPEN network
 *                - an ASCII string, length is [8, 63]
 *                - a hex string (two characters per octet of PSK), length is 64
 * @return 0 on success, -1 on failure
 *
 * @note This way is only adapted to join the OPEN, WPA/WPA2/WPA3-PSK network.
 */
static __inline int wlan_sta_set(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk)
{
	return wlan_sta_config(ssid, ssid_len, psk, WLAN_STA_CONF_FLAG_WPA3);
}

/**
 * @brief Set station specified field configuration
 * @param[in] config Pointer to the configuration
 * @return 0 on success, -1 on failure
 */
int wlan_sta_set_config(wlan_sta_config_t *config);

/**
 * @brief Get station specified field configuration
 * @param[in] config Pointer to the configuration
 * @return 0 on success, -1 on failure
 */
int wlan_sta_get_config(wlan_sta_config_t *config);

/**
 * @brief Set autoconnect after bss lost configuration
 * @param[in] config enable or disable autoconnect function
 * @return 0 on success, -1 on failure
 */
int wlan_sta_set_autoconnect(int enable);

/**
 * @brief Get the information size of current bss
 * @param[in] config Pointer to the information size
 * @return 0 on success, -1 on failure
 */
int wlan_sta_get_bss_size(uint32_t *size);

/**
 * @brief Get the information of current bss
 * @param[in] config Pointer to the information
 * @return 0 on success, -1 on failure
 */
int wlan_sta_get_bss(wlan_sta_bss_info_t *bss_get);

/**
 * @brief Set the information of bss which will be used
 * @param[in] config Pointer to the information of bss
 * @return 0 on success, -1 on failure
 */
int wlan_sta_set_bss(wlan_sta_bss_info_t *bss_set);

/**
 * @brief Enable the station
 * @return 0 on success, -1 on failure
 */
int wlan_sta_enable(void);

/**
 * @brief Disable the station
 * @return 0 on success, -1 on failure
 */
int wlan_sta_disable(void);

/**
 * @brief Station scan once according to the default parameters
 * @return 0 on success, -1 on failure
 */
int wlan_sta_scan_once(void);

/**
 * @brief Get station scan results' number
 * @param[in] num Pointer to the number of scan results' number
 * @return 0 on success, -1 on failure
 */
int wlan_sta_get_scan_result_num(int *num);

/**
 * @brief Station scan once according to the specified parameters
 * @return 0 on success, -1 on failure
 */
int wlan_sta_scan(wlan_sta_scan_param_t *param);

/**
 * @brief Get station scan results
 * @param[in] results Pointer to the scan results
 * @return 0 on success, -1 on failure
 */
int wlan_sta_scan_result(wlan_sta_scan_results_t *results);

/**
 * @brief Set station scan interval
 * @param[in] sec Scan interval in Seconds
 * @return 0 on success, -1 on failure
 */
int wlan_sta_scan_interval(int sec);

/**
 * @brief Set maximum BSS entries to keep in memory
 * @param[in] count Maximum BSS entries to keep in memory
 * @return 0 on success, -1 on failure
 */
int wlan_sta_bss_max_count(uint8_t count);

/**
 * @brief Flush station old BSS entries
 * @param[in] age Maximum entry age in seconds
 * @return 0 on success, -1 on failure
 *
 * @note Remove BSS entries that have not been updated during the last @age
 * seconds.
 */
int wlan_sta_bss_flush(int age);

/**
 * @brief Request a new connection
 * @return 0 on success, -1 on failure
 */
int wlan_sta_connect(void);

/**
 * @brief Disconnect the current connection
 * @return 0 on success, -1 on failure
 */
int wlan_sta_disconnect(void);

/**
 * @brief Get station connection state
 * @param[in] state Pointer to the connection state
 * @return 0 on success, -1 on failure
 */
int wlan_sta_state(wlan_sta_states_t *state);

/**
 * @brief Get the information of connected AP
 * @param[in] ap Pointer to the AP information
 * @return 0 on success, -1 on failure
 */
int wlan_sta_ap_info(wlan_sta_ap_t *ap);

/**
 * @brief Generate WPA PSK based on passphrase and SSID
 * @param[in] param Pointer to wlan_gen_psk_param_t structure
 * @return 0 on success, -1 on failure
 */
int wlan_sta_gen_psk(wlan_gen_psk_param_t *param);

/**
 * @brief Get current SSID, passphrase and WPA PSK of the connected AP
 * @param[in] info Pointer to wlan_ssid_psk_t structure
 * @return 0 on success, -1 on failure
 */
int wlan_sta_get_ap_ssid_psk(wlan_ssid_psk_t *info);

/**
 * @brief Start the WPS negotiation with PBC method
 * @return 0 on success, -1 on failure
 *
 * @note WPS will be turned off automatically after two minutes.
 */
int wlan_sta_wps_pbc(void);

/**
 * @brief Get a random valid WPS PIN
 * @param[in] wps Pointer to the WPS pin
 * @return 0 on success, -1 on failure
 */
int wlan_sta_wps_pin_get(wlan_sta_wps_pin_t *wps);

/**
 * @brief Start the WPS negotiation with PIN method
 * @param[in] wps Pointer to the WPS pin
 * @return 0 on success, -1 on failure
 *
 * @note WPS will be turned off automatically after two minutes.
 */
int wlan_sta_wps_pin_set(wlan_sta_wps_pin_t *wps);

/* softAP */

void wlan_ap_set_default_conf(const wlan_ap_default_conf_t *conf);

const wlan_ap_default_conf_t *wlan_ap_get_default_conf(void);

/**
 * @brief Configure AP in a convenient way to build a specified network
 * @param[in] ssid Network name, length is [1, 32]
 * @param[in] ssid_len The length of network name
 * @param[in] psk Network password, in one of the optional formats:
 *                - NULL or an empty string, to build an OPEN network
 *                - an ASCII string, length is [8, 63]
 *                - a hex string (two characters per octet of PSK), length is 64
 * @return 0 on success, -1 on failure
 *
 * @note This way is only adapted to build an OPEN or WPA-PSK/WPA2-PSK network.
 */
int wlan_ap_set(uint8_t *ssid, uint8_t ssid_len, uint8_t *psk);

/**
 * @brief Set AP specified field configuration
 * @param[in] config Pointer to the configuration
 * @return 0 on success, -1 on failure
 */
int wlan_ap_set_config(wlan_ap_config_t *config);

/**
 * @brief Get AP specified field configuration
 * @param[in] config Pointer to the configuration
 * @return 0 on success, -1 on failure
 */
int wlan_ap_get_config(wlan_ap_config_t *config);

/**
 * @brief Enable the AP
 * @return 0 on success, -1 on failure
 */
int wlan_ap_enable(void);

/**
 * @brief Reload AP configuration
 * @return 0 on success, -1 on failure
 */
int wlan_ap_reload(void);

/**
 * @brief Disable the AP
 * @return 0 on success, -1 on failure
 */
int wlan_ap_disable(void);

/**
 * @brief Get the number of connected stations
 * @param[in] num Pointer to the number
 * @return 0 on success, -1 on failure
 */
int wlan_ap_sta_num(int *num);

/**
 * @brief Get the information of connected stations
 * @param[in] stas Pointer to the stations information
 * @return 0 on success, -1 on failure
 */
int wlan_ap_sta_info(wlan_ap_stas_t *stas);

/**
 * @brief ap scan once according to the default parameters
 * @return 0 on success, -1 on failure
 */
int wlan_ap_scan_once(void);

/**
 * @brief ap scan once according to the specified parameters
 * @return 0 on success, -1 on failure
 */
int wlan_ap_scan(wlan_sta_scan_param_t *param);

/**
 * @brief Get ap scan results' number
 * @param[in] num Pointer to the number of scan results' number
 * @return 0 on success, -1 on failure
 */
int wlan_ap_get_scan_result_num(int *num);

/**
 * @brief Get ap scan results
 * @param[in] results Pointer to the scan results
 * @return 0 on success, -1 on failure
 */
int wlan_ap_scan_result(wlan_sta_scan_results_t *results);

/**
 * @brief Set maximum BSS entries to keep in memory
 * @param[in] count Maximum BSS entries to keep in memory
 * @return 0 on success, -1 on failure
 */
int wlan_ap_scan_bss_max_count(uint8_t count);

/**
 * @brief Flush ap old BSS entries
 * @param[in] age Maximum entry age in seconds
 * @return 0 on success, -1 on failure
 *
 * @note Remove BSS entries that have not been updated during the last @age
 * seconds.
 */
int wlan_ap_bss_flush(int age);

/* monitor */
typedef enum {
	AUTH_ALG_OPEN,
	AUTH_ALG_SHARED,
	AUTH_ALG_LEAP,
} auth_alg;

struct frame_info {
	uint16_t recv_channel;  /* frame receved channel */
	uint16_t ap_channel;    /* ap channel if the frame is beacon or probe response frame */
	uint8_t type;
	uint8_t rssi;
	union {
		/* for multi core heterogeneous compatibility */
		auth_alg alg;
		uint32_t alg32;
	};
};

typedef void (*wlan_monitor_rx_cb)(uint8_t *data, uint32_t len, void *info);
typedef void (*wlan_monitor_sw_channel_cb)(struct netif *nif, int16_t channel);
int wlan_monitor_set_rx_cb(struct netif *nif, wlan_monitor_rx_cb cb);
int wlan_monitor_set_sw_channel_cb(struct netif *nif, wlan_monitor_sw_channel_cb cb);
int wlan_monitor_set_channel(struct netif *nif, int16_t channel);
int wlan_monitor_input(struct netif *nif, uint8_t *data, uint32_t len, void *info);
int wlan_send_raw_frame(struct netif *netif, int type, uint8_t *buffer, int len);

typedef enum wlan_p2p_state {
	WLAN_P2P_STATE_INIT,
	WLAN_P2P_STATE_NORMAL,
	WLAN_P2P_STATE_WAKE_UP,
	WLAN_P2P_STATE_KPALIVE_LOSS,
	WLAN_P2P_STATE_MAX,
} wlan_p2p_state_t;
wlan_p2p_state_t wlan_p2p_state_get(void);

typedef enum wlan_auto_scan_state {
	WLAN_AUTO_SCAN_STATE_INIT,
	WLAN_AUTO_SCAN_STATE_FOUND_AP,
	WLAN_AUTO_SCAN_STATE_MAX,
} wlan_auto_scan_state_t;
wlan_auto_scan_state_t wlan_auto_scan_state_get(void);

typedef enum wlan_sniffer_sync_state {
	WLAN_SNIF_SYNC_STATE_SUCCESS,
	WLAN_SNIF_SYNC_STATE_FAILURE,
	WLAN_SNIF_SYNC_STATE_RECV_SYNC_FRM,
	WLAN_SNIF_SYNC_STATE_LOST_1ST_SYNC_FRM,
	WLAN_SNIF_SYNC_STATE_LOST_SYNC_FRM_MAX,
	WLAN_SNIF_SYNC_STATE_INIT,
} wlan_sniffer_sync_state_t;
wlan_sniffer_sync_state_t wlan_sniffer_sync_state_get(void);

void *wlan_if_create(enum wlan_mode mode, struct netif *nif, const char *name);
int wlan_if_delete(void *ifp);

struct mbuf;
int wlan_linkoutput(struct netif *nif, struct mbuf *m);

#ifdef CONFIG_ARCH_SUN20IW2P1
int wlan_sta_set_ssid_prb_only(uint32_t enable);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _NET_WLAN_WLAN_H_ */
