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

#ifndef _NET_WLAN_WLAN_EXT_REQ_H_
#define _NET_WLAN_WLAN_EXT_REQ_H_

#include <stdint.h>
//#ifndef CONFIG_OS_NUTTX
#include "lwip/netif.h"
//#endif
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Wlan extended command definition
 */
typedef enum wlan_ext_cmd {
	WLAN_EXT_CMD_SET_PM_DTIM = 0,
	WLAN_EXT_CMD_GET_PM_DTIM,
	WLAN_EXT_CMD_SET_PS_CFG,
	WLAN_EXT_CMD_SET_AMPDU_TXNUM,
	WLAN_EXT_CMD_SET_TX_RETRY_CNT,
	WLAN_EXT_CMD_SET_PM_TX_NULL_PERIOD,
	WLAN_EXT_CMD_SET_BCN_WIN_US,
	WLAN_EXT_CMD_GET_BCN_STATUS,
	WLAN_EXT_CMD_SET_PHY_PARAM,
	WLAN_EXT_CMD_SET_SCAN_PARAM,
	WLAN_EXT_CMD_SET_LISTEN_INTERVAL,//10
	WLAN_EXT_CMD_SET_AUTO_SCAN,
	WLAN_EXT_CMD_SET_P2P_SVR,
	WLAN_EXT_CMD_SET_P2P_WKP_CFG,
	WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG,
	WLAN_EXT_CMD_SET_P2P_HOST_SLEEP,
	WLAN_EXT_CMD_SET_BCN_WIN_CFG,
	WLAN_EXT_CMD_SET_FORCE_B_RATE,
	WLAN_EXT_CMD_SET_RCV_SPECIAL_FRM,
	WLAN_EXT_CMD_SET_SEND_RAW_FRM_CFG,
	WLAN_EXT_CMD_SET_SNIFF_SYNC_CFG,//20
	WLAN_EXT_CMD_SET_RCV_FRM_FILTER_CFG,
	WLAN_EXT_CMD_SET_POWER_LEVEL_TAB,
	WLAN_EXT_CMD_GET_POWER_LEVEL_TAB,
	WLAN_EXT_CMD_SET_SWITCH_CHN_CFG,
	WLAN_EXT_CMD_GET_CURRENT_CHN,
	WLAN_EXT_CMD_SET_SNIFF_KP_ACTIVE,
	WLAN_EXT_CMD_SET_FRM_FILTER,
	WLAN_EXT_CMD_SET_TEMP_FRM,
	WLAN_EXT_CMD_SET_UPDATE_TEMP_IE,
	WLAN_EXT_CMD_SET_SYNC_FRM_SEND,//30
	WLAN_EXT_CMD_SET_SNIFF_EXTERN_CFG,
	WLAN_EXT_CMD_GET_SNIFF_STAT,
	WLAN_EXT_CMD_GET_TEMP_VOLT,
	WLAN_EXT_CMD_SET_CHANNEL_FEC,
	WLAN_EXT_CMD_SET_TEMP_VOLT_AUTO_UPLOAD,
	WLAN_EXT_CMD_SET_TEMP_VOLT_THRESH,
	WLAN_EXT_CMD_SET_EDCA_PARAM,
	WLAN_EXT_CMD_GET_EDCA_PARAM,
	WLAN_EXT_CMD_GET_STATS_CODE,
	WLAN_EXT_CMD_SET_BCN_FREQ_OFFS_TIME,
	WLAN_EXT_CMD_SET_LFCLOCK_PARAM,

	WLAN_EXT_CMD_SET_SDD_FREQ_OFFSET,
	WLAN_EXT_CMD_GET_SDD_FREQ_OFFSET,
	WLAN_EXT_CMD_SET_SDD_POWER,
	WLAN_EXT_CMD_GET_SDD_POWER,
	WLAN_EXT_CMD_GET_SDD_FILE,

	WLAN_EXT_CMD_GET_TX_RATE = 50,
	WLAN_EXT_CMD_GET_SIGNAL,
	WLAN_EXT_CMD_SET_MIXRATE,

	WLAN_EXT_CMD_SET_MBUF_LIMIT,
	WLAN_EXT_CMD_SET_AMPDU_REORDER_AGE,
	WLAN_EXT_CMD_SET_SCAN_FREQ,
	WLAN_EXT_CMD_SET_RX_STACK_SIZE, /* Should be called before wlan_attach() */
	WLAN_EXT_CMD_SET_RX_QUEUE_SIZE, /* Should be called before wlan_attach() */
	WLAN_EXT_CMD_SET_AMRR_PARAM,
	WLAN_EXT_CMD_SET_BCN_RX_11B_ONLY,
	WLAN_EXT_CMD_AUTO_BCN_OFFSET_SET,
	WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ,
	WLAN_EXT_CMD_SET_SNIFFER, /* set sniffer param */
	WLAN_EXT_CMD_SET_MIMO_PARAM,
	WLAN_EXT_CMD_SET_PS_POLICY,
	WLAN_EXT_CMD_SET_PRE_RX_BCN,
	WLAN_EXT_CMD_SET_STAY_AWAKE_TMO,
	WLAN_EXT_CMD_SET_BCN_WITHOUT_DATA,
	WLAN_EXT_CMD_SET_BCN_TIM_NO_DATA_TMO,
	WLAN_EXT_CMD_GET_BCN_RSSI_DBM,
	WLAN_EXT_CMD_SET_BSS_LOSS_THOLD,
	WLAN_EXT_CMD_SET_UPLOAD_PRB_REQ,
	WLAN_EXT_CMD_SET_AUTH_TMO_AND_TRIES,
	WLAN_EXT_CMD_SET_ASSOC_TMO_AND_TRIES,
	WLAN_EXT_CMD_SET_AUTO_POWER,
	WLAN_EXT_CMD_SET_BCN_LOST_COMP,
	WLAN_EXT_CMD_SET_ARP_KPALIVE,
	WLAN_EXT_CMD_SET_FILTER_TYPE,
} wlan_ext_cmd_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_PS_CFG
 */
typedef struct wlan_ext_ps_cfg {
	uint8_t ps_mode;
	uint8_t ps_idle_period;
	uint8_t ps_change_period;
} wlan_ext_ps_cfg_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_GET_PM_DTIM
 */
typedef struct wlan_ext_pm_dtim {
	uint8_t pm_join_dtim_period;
	uint8_t pm_dtim_period_extend;
} wlan_ext_pm_dtim_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_GET_BCN_STATUS
 */
typedef struct wlan_ext_bcn_status {
	uint32_t bcn_duration;
	int32_t  bcn_delay_max;
	int32_t  bcn_delay_sum;
	uint16_t bcn_delay_cnt[8];
	uint16_t bcn_rx_cnt;
	uint16_t bcn_miss_cnt;
} wlan_ext_bcn_status_t;

typedef struct wlan_ext_bcn_dly {
	uint32_t bcn_handle_time;
	uint32_t phy_start_end_time;
	uint32_t bcn_recive_time;
	uint32_t bcn_duration_time;
	uint32_t bcn_dtim_time;
} wlan_ext_bcn_dly_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_AUTO_BCN_OFFSET_SET and WLAN_EXT_CMD_AUTO_BCN_OFFSET_READ
 */
#define BCN_AUTO_OFFSET_DLY_TIME_CNT    20
typedef struct wlan_ext_bcn_auto_offset {
//  host set
	uint8_t auto_offset_mode;
	uint8_t auto_offset_percentage;
	uint8_t bcn_statistics;
	uint8_t auto_offset_open_threshold;
	uint8_t bcn_lost_max_num;
//  host read
	uint8_t bcn_dly_cnt[8];
	uint8_t bcn_dly_all_cnt;
	uint8_t auto_offset_open;
	uint16_t auto_offset_val;
	uint16_t bcn_auto_offset_open_cnt;

	int16_t old_bcn_freq_offset_adj;
	int16_t new_bcn_freq_offset_adj;
	uint8_t bcn_dly_time_cnt;
	int16_t bcn_dly_time[BCN_AUTO_OFFSET_DLY_TIME_CNT];
	uint32_t old_bcn_win_timeout;
} wlan_ext_bcn_auto_offset_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_GET_SIGNAL
 */
typedef struct wlan_ext_signal {
	int16_t rssi;    /* snr, unit is 0.5db */
	int8_t noise;   /* unit is dbm */
} wlan_ext_signal_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_MBUF_LIMIT
 */
typedef struct wlan_ext_mbuf_limit {
	uint32_t tx;
	uint32_t rx;
	uint32_t txrx;
} wlan_ext_mbuf_limit_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_PHY_PARAM
 */
typedef struct wlan_ext_phy_param {
	uint8_t check_period;       /* stun checking period (in ms)
	                               MUST be divisible by 200 (required by fw) */
	uint8_t cca_threshold;      /* cca stun threshold */
	uint8_t ofdm_threshold;     /* ofdm stun threshold */
	uint8_t ofdm_rate_idx;      /* h/w rate index for transmitting null frame */
	uint8_t ofdm_retry_cnt;     /* retry count for transmitting null frame */
	uint8_t ofdm_max_interval;  /* max interval (in seconds) to applying ofdm
	                               workaround, 0 for no workaround */
} wlan_ext_phy_param_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SCAN_PARAM
 */
typedef struct wlan_ext_scan_param {
	uint8_t  num_probes;  /* number of probe requests (per SSID) sent to
	                         one channel, default to 2 */
	uint8_t  probe_delay; /* delay time (in us) before sending a probe request,
	                         default to 100 us */
	uint16_t min_dwell;   /* min channel dwell time (in ms), default to 15 ms */
	uint16_t max_dwell;   /* max channel dwell time (in ms), default to 35 ms */
} wlan_ext_scan_param_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SCAN_FREQ
 */
typedef struct wlan_ext_scan_freq {
	uint16_t freq_num;
	union {
		int32_t  *freq_list;
		uint64_t freq_list64;
	};
} wlan_ext_scan_freq_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_AUTO_SCAN
 */
typedef struct wlan_ext_auto_scan_param {
	uint8_t  auto_scan_enable;  /* enable auto scan, default disable(0) */
	uint32_t auto_scan_interval; /* auto scan interval(in ms), defualt 0ms */
} wlan_ext_auto_scan_param_t;

typedef enum wlan_ext_auto_scan_state {
	WLAN_EXT_AUTO_SCAN_STATE_INIT,
	WLAN_EXT_AUTO_SCAN_STATE_FOUND_AP,
	WLAN_EXT_AUTO_SCAN_STATE_MAX,
} wlan_ext_auto_scan_state_t;
void wlan_ext_auto_scan_state_set(wlan_ext_auto_scan_state_t state);
wlan_ext_auto_scan_state_t wlan_ext_auto_scan_state_get(void);

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_P2P_SVR
 */
#define IPC_P2P_KPALIVE_PAYLOAD_LEN_MAX 128
#define IPC_P2P_WUP_PAYLOAD_LEN_MAX     128
#define IPC_P2P_SERVER_MAX 3
#define IPC_P2P_IPV4_FILTER_NUM_MAX     8
typedef struct wlan_ext_p2p_svr {
	uint16_t  Enable;
	uint16_t  IPIdInit;
	uint32_t  TcpSeqInit;
	uint32_t  TcpAckInit;
	uint8_t   DstIPv4Addr[4];
	uint16_t  SrcPort;
	uint16_t  DstPort;
	uint8_t   DstMacAddr[6];
	uint16_t  TcpOrUdp;//0x01:tcp  0x02:udp  0x03:both
} wlan_ext_p2p_svr_t;

typedef struct wlan_ext_p2p_svr_set {
	uint8_t   EncHdrSize;
	uint8_t   EncTailSize;
	uint16_t  reserved1;
	uint8_t   SrcIPv4Addr[4];
	wlan_ext_p2p_svr_t  P2PServerCfgs[IPC_P2P_SERVER_MAX];
} wlan_ext_p2p_svr_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_P2P_WKP_CFG
 */
typedef struct wlan_ext_p2p_wkp_param_cfg {
	uint16_t  Enable;
	uint16_t  PayloadLen;
	uint8_t   Payload[IPC_P2P_WUP_PAYLOAD_LEN_MAX];
} wlan_ext_p2p_wkp_param_cfg_t;

typedef struct wlan_ext_p2p_ipv4_filter_cfg {
	uint8_t   Ipv4Filter[4];
} wlan_ext_p2p_ipv4_filter_cfg_t;

typedef struct wlan_ext_p2p_wkp_param_set {
	wlan_ext_p2p_wkp_param_cfg_t P2PWkpParamCfgs[IPC_P2P_SERVER_MAX];
	uint32_t   Enable;
	wlan_ext_p2p_ipv4_filter_cfg_t P2PIpv4FilterCfgs[IPC_P2P_IPV4_FILTER_NUM_MAX];
} wlan_ext_p2p_wkp_param_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_P2P_KPALIVE_CFG
 */
typedef struct wlan_ext_p2p_kpalive_param_cfg {
	uint16_t  Enable;
	uint16_t  PayloadLen;
	uint8_t   Payload[IPC_P2P_KPALIVE_PAYLOAD_LEN_MAX];
} wlan_ext_p2p_kpalive_param_cfg_t;

typedef struct wlan_ext_p2p_kpalive_param_set {
	uint8_t   KeepAlivePeriod_s;      // unit:Second
	uint8_t   TxTimeOut_s;            // unit:Second  Keep alive packet tx timeout value
	uint8_t   TxRetryLimit;           // keep alive packet tx retry limit
	uint8_t   reserved1;
	wlan_ext_p2p_kpalive_param_cfg_t P2PKeepAliveParamCfgs[IPC_P2P_SERVER_MAX];
} wlan_ext_p2p_kpalive_param_set_t;

typedef enum wlan_ext_p2p_state {
	WLAN_EXT_P2P_STATE_INIT,
	WLAN_EXT_P2P_STATE_NORMAL,
	WLAN_EXT_P2P_STATE_WAKE_UP,
	WLAN_EXT_P2P_STATE_KPALIVE_LOSS,
	WLAN_EXT_P2P_STATE_MAX,
} wlan_ext_p2p_state_t;
void wlan_ext_p2p_state_set(wlan_ext_p2p_state_t state);
wlan_ext_p2p_state_t wlan_ext_p2p_state_get(void);

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_RCV_FRM_FILTER_CFG
 */
#define RCV_FRM_FILTER_FRAME_TYPE      (1 << 0)
#define RCV_FRM_FILTER_MAC_ADDRESS     (1 << 1)
#define RCV_FRM_FILTER_PAYLOAD         (1 << 2)
#define RCV_FRM_FILTER_IE              (1 << 3)

#define RCV_FRM_FILTER_MAC_ADDR_A1     (1 << 0)
#define RCV_FRM_FILTER_MAC_ADDR_A2     (1 << 1)
#define RCV_FRM_FILTER_MAC_ADDR_A3     (1 << 2)

//FrameType define
#define FILTER_D11_MGMT_TYPE           0
#define FILTER_D11_SUB_MGMT_ASRQ       (1 << (0x0 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_ASRSP      (1 << (0x1 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_RSRQ       (1 << (0x2 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_RSRSP      (1 << (0x3 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_PBRQ       (1 << (0x4 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_PBRSP      (1 << (0x5 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_BCN        (1 << (0x8 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_ATIM       (1 << (0x9 + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_DAS        (1 << (0xa + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_AUTH       (1 << (0xb + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_DAUTH      (1 << (0xc + FILTER_D11_MGMT_TYPE))
#define FILTER_D11_SUB_MGMT_ACTION     (1 << (0xd + FILTER_D11_MGMT_TYPE))

#define FILTER_D11_DATA_TYPE           16
#define FILTER_D11_SUB_DATA            (1 << (0x0 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DATA_CFACK      (1 << (0x1 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DATA_CFPOLL     (1 << (0x2 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DATA_CFAKPL     (1 << (0x3 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DNUL            (1 << (0x4 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DNUL_CFACK      (1 << (0x5 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DNUL_CFPOLL     (1 << (0x6 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_DNUL_CFAKPL     (1 << (0x7 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDATA           (1 << (0x8 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDATA_CFACK     (1 << (0x9 + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDATA_CFPOLL    (1 << (0xa + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDATA_CFAKPL    (1 << (0xb + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDNUL           (1 << (0xc + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDNUL_CFACK     (1 << (0xd + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDNUL_CFPOLL    (1 << (0xe + FILTER_D11_DATA_TYPE))
#define FILTER_D11_SUB_QDNUL_CFAKPL    (1 << (0xf + FILTER_D11_DATA_TYPE))

#define RCV_FRM_FILTER_PAYLOAD_LEN_MAX 256
#define RCV_FRM_FILTER_IE_LEN_MAX      256
typedef struct rcv_frm_filter {
	uint16_t  FilterEnable;
	uint16_t  AndOperationMask;
	uint16_t  OrOperationMask;
	uint16_t  Reserved;
	uint32_t  FrameType;
	uint8_t   MacAddrMask;
	uint8_t   Reserved1;
	uint8_t   MacAddrA1[6];
	uint8_t   MacAddrA2[6];
	uint8_t   MacAddrA3[6];
	union{
		struct {
			uint16_t  PayloadOffset;
			uint16_t  PayloadLength;
			uint8_t   Payload[RCV_FRM_FILTER_PAYLOAD_LEN_MAX];
		} __packed PayloadCfg;
		struct {
			uint8_t  ElementId;
			uint8_t  Length;
			uint16_t Reserved;
			uint8_t  OUI[RCV_FRM_FILTER_IE_LEN_MAX];
		} __packed IeCfg;
	} __packed;
} rcv_frm_filter_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_FRM_FILTER
 */
#define FRAME_FILTER_ENABLE           (1 << 0)
typedef struct wlan_ext_frm_filter_set {
	uint32_t  Filter1Cfg;
	uint32_t  Filter2Cfg;
	rcv_frm_filter_t  Filter1;
	rcv_frm_filter_t  Filter2;
} wlan_ext_frm_filter_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_RCV_SPECIAL_FRM
 */
#define SEND_DUPLICATE_FRAME_TO_HOST_ENABLE    (1 << 0)
#define RECV_UNICAST_FRAME_ENABLE              (1 << 1)
#define RECV_BROADCAST_FRAME_ENABLE            (1 << 2)

typedef struct wlan_ext_rcv_spec_frm_param_set {
	uint32_t  Enable; //0 or 1
	uint32_t  u32RecvSpecFrameCfg;
} wlan_ext_rcv_spec_frm_param_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SEND_RAW_FRM_CFG
 */
#define SEND_RAW_FRAME_ALLOCATE_SEQNUM        (1 << 0)    //wifi allocate tx frame sequence number, not from Host
#define SEND_RAW_FRAME_NO_ACK                 (1 << 1)    //wifi will not wait for ack after send raw frame if bit is be set
typedef struct wlan_ext_send_raw_frm_param_set {
	uint8_t     Enable;
	uint8_t     Reserved;
	uint16_t    u16SendRawFrameCfg;//reserved for now
} wlan_ext_send_raw_frm_param_set_t;

//Band
#define SEND_RAW_FRAME_BAND_2G4               0
#define SEND_RAW_FRAME_BAND_5G                1

//Flag
#define SEND_RAW_FRAME_FLAG_SHORT_PREAMBLE   (1 << 2)//no use now
#define SEND_RAW_FRAME_USE_MAC_ADDR_1        (1 << 4)//no use now

#define SEND_RAW_FRAME_MAX_SWITCH_CHANNEL_TIME   5000
typedef struct wlan_ext_switch_channel_param_set {
	uint8_t     Enable;
	uint8_t     Band;
	uint16_t    Flag;//reserved now
	uint32_t    ChannelNum;
	uint32_t    SwitchChannelTime;
} wlan_ext_switch_channel_param_set_t;

typedef struct wlan_ext_get_cur_channel {
	uint16_t    Channel;
	uint16_t    Reserved;
} wlan_ext_get_cur_channel_t;


/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SNIFF_SYNC_CFG
 */
//u32SniffSyncCfg
#define SNIFF_SYNC_AT_HOST              (0x1)
#define SNIFF_SYNC_AT_WIFI              (0x0)
#define SNIFF_SYNC_METHOD               (1 << 0)
#define SNIFF_SYNC_DUPLICATE_TO_HOST    (1 << 1)
#define SNIFF_SYNC_DISABLE_TIMER        (1 << 2)
#define SNIFF_SYNC_UPLOAD_DATA_FRM      (1 << 3)
#define SNIFF_SYNC_UNICAST_FRM          (1 << 4)
#define SNIFF_SYNC_MULTICAST_FRM        (1 << 5)

//SYNC_AT_HOST:Flag
#define SNIFF_SYNC_AT_HOST_FRAME_SEND_TO_HOST         (1 << 0) //send frame to host if enable

//SYNC_AT_WIFI:Flag
#define SNIFF_SYNC_AT_WIFI_FRAME_SEND_TO_HOST         (1 << 0)
#define SNIFF_SYNC_AT_WIFI_SEND_HOST_LOST_SYNC        (1 << 1)
#define SNIFF_SYNC_AT_WIFI_ADAPTIVE_EXPANSION_ENABLE  (1 << 2)
#define SNIFF_SYNC_AT_WIFI_SEND_SYNC_INDC_TO_HOST     (1 << 3)
#define SNIFF_SYNC_AT_WIFI_UPLOAD_WHEN_PL_MISMATCH    (1 << 4)
#define SNIFF_SYNC_AT_WIFI_SYNC_USE_FILTER1           (1 << 5)
#define SNIFF_SYNC_AT_WIFI_SYNC_USE_FILTER2           (1 << 6)
#define SNIFF_SYNC_AT_WIFI_WAKEUP_ADVANCE_ENABLE      (1 << 7)

typedef struct wlan_ext_sniff_sync_param_set {
	uint8_t   Enable;
	uint8_t   ChannelNum;
	uint16_t  Reserved0;
	uint32_t  SyncFrameType;
	uint32_t  u32SniffSyncCfg;
	union {
		struct {
			uint32_t  WakeupPeriod_ms;       //unit:millisecond
			uint32_t  KeepActivePeriod_ms;   //unit:millisecond
			uint8_t   Flag;
			uint8_t   Reserved1;
			uint16_t  Reserved2;
			uint32_t  StartTime;            //unit:millisecond
		} __packed time_sync_at_host;
		struct {
			uint8_t   Flag;
			uint8_t   SyncDTIM;
			uint8_t   MaxLostSyncPacket;
			uint8_t   TSFOffset;                  //unit:byte
			uint8_t   AdaptiveExpansion;          //unit:millisecond
			uint8_t   KeepActiveNumAfterLostSync; //unit:millisecond
			uint16_t  ActiveTime;                 //unit:millisecond
			uint8_t   MaxAdaptiveExpansionLimit;  //unit:millisecond
			uint8_t   WakeupAdvanceTime;          //unit:millisecond
			uint16_t  MaxKeepAliveTime;           //unit:millisecond
		} __packed time_sync_at_wifi;
	} __packed;
} wlan_ext_sniff_sync_param_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SNIFF_KP_ACTIVE
 */
//u32Config
#define SNIFF_FRM_ALLOCATE_SEQ_NUM       (1 << 0)
typedef struct wlan_ext_sniff_kp_active_set {
	uint32_t  Enable;
	uint32_t  u32Config;
} wlan_ext_sniff_kp_active_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_TEMP_FRM
 */
//u32Config
#define MAX_FRM_LEN       (694)
typedef struct wlan_ext_temp_frm_set {
	uint16_t  FrmLength;
	uint8_t   Frame[MAX_FRM_LEN];
} wlan_ext_temp_frm_set_t;

/* for set update tmp ie */
#define IE_NUM 7

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SYNC_FRM_SEND
 */
#define SEND_SYNC_FRM_ADVANCE_ENABLE        (1 << 0)
typedef struct wlan_ext_send_sync_frm_set {
	uint8_t   Enable;
	uint8_t   SendSyncFrmAdvanceTime;//unit:ms
	uint16_t  Flag;
	uint32_t  BcnInterval;
} wlan_ext_send_sync_frm_set_t;//units:1024us

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SNIFF_EXTERN_CFG
 */
//SniffExtFuncEnable
#define SNIFF_ADAPT_EXPEN_DIFF_ENABLE       (1 << 0)
typedef struct wlan_ext_sniff_extern_param_set {
	uint32_t    SniffExtFuncEnable;
	uint32_t    Reserved0;
	uint8_t     SniffRetryAfterLostSync;
	uint8_t     SniffAdaptiveExpenRight;
	uint8_t     SniffRetryDtim;
	uint8_t     Reserved1;
} wlan_ext_sniff_extern_param_set_t;

typedef enum wlan_ext_sniffer_sync_state {
	WLAN_EXT_SNIF_SYNC_STATE_SUCCESS,
	WLAN_EXT_SNIF_SYNC_STATE_FAILURE,
	WLAN_EXT_SNIF_SYNC_STATE_RECV_SYNC_FRM,
	WLAN_EXT_SNIF_SYNC_STATE_LOST_1ST_SYNC_FRM,
	WLAN_EXT_SNIF_SYNC_STATE_LOST_SYNC_FRM_MAX,
	WLAN_EXT_SNIF_SYNC_STATE_INIT,
} wlan_ext_sniffer_sync_state_t;
void wlan_ext_sniffer_sync_state_set(wlan_ext_sniffer_sync_state_t state);
wlan_ext_sniffer_sync_state_t wlan_ext_sniffer_sync_state_get(void);

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_BCN_WIN_CFG
 */
typedef struct wlan_ext_bcn_win_param_set {
	uint32_t  BeaconWindowAdjAmpUs;
	uint8_t   BeaconWindowAdjStartNum;
	uint8_t   BeaconWindowAdjStopNum;
	uint8_t   BeaconWindowMaxStartNum;
	uint8_t   Reserved;
} wlan_ext_bcn_win_param_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_FORCE_B_RATE
 */
typedef struct wlan_ext_force_b_rate_set {
	uint8_t   ForceBRateEnable;
	uint8_t   Force2mThreshold;
	uint8_t   Force1mThreshold;
	uint8_t   Reserved;
} wlan_ext_force_b_rate_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_POWER_LEVEL_TAB
 */
#define POWER_LEVEL_TAB_USE_LENGTH     11
typedef struct wlan_ext_power_level_tab_set {
	uint16_t   PowerTab[POWER_LEVEL_TAB_USE_LENGTH];
} wlan_ext_power_level_tab_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_GET_POWER_LEVEL_TAB
 */
#define POWER_LEVEL_TAB_TYPE_MAX       0
#define POWER_LEVEL_TAB_TYPE_CUR       1
#define POWER_LEVEL_TAB_TYPE_USER      2
typedef struct wlan_ext_power_level_tab_get {
	uint16_t   PowerTabType;
	uint16_t   PowerTab[POWER_LEVEL_TAB_USE_LENGTH];
} wlan_ext_power_level_tab_get_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_GET_TEMP_VOLT
 */
typedef struct wlan_ext_temp_volt_get {
	int32_t    Temperature;
	uint32_t   Voltage;
} wlan_ext_temp_volt_get_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_TEMP_VOLT_AUTO_UPLOAD
 */
typedef struct wlan_ext_temp_volt_auto_upload_set {
	uint16_t   Enable;
	uint16_t   UploadPeriod;
} wlan_ext_temp_volt_auto_upload_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_TEMP_VOLT_THRESH
 */
typedef struct wlan_ext_temp_volt_thresh_set {
	uint8_t    TempHighEn;     /* enable temperature high thresh */
	uint8_t    TempLowEn;      /* enable temperature low thresh */
	uint8_t    VoltHighEn;     /* enable voltage high thresh */
	uint8_t    VoltLowEn;      /* enable voltage low thresh */
	int32_t    TempHighThresh; /* thresh high value for temperature */
	int32_t    TempLowThresh;  /* thresh low value for temperature */
	uint32_t   VoltHighThresh; /* thresh high value for voltage */
	uint32_t   VoltLowThresh;  /* thresh low value for voltage */

	/****** Ext Param Below, only set when necessary ******/
	uint8_t    TempJitterCnt;          /* only report when temp keep over thresh for more than TempJitterCnt cycles */
	uint8_t    VoltJitterCnt;          /* only report when volt keep over thresh for more than VoltJitterCnt cycles */
	uint8_t    TempUseDeltaEn;         /* thresh value is regard as delta value, the base value is the last reported value */
	uint8_t    VoltUseDeltaEn;         /* thresh value is regard as delta value, the base value is the last reported value */
	uint8_t    TempVoltFixedRefEn;     /* keep base value fixed, only use when XXXXUseDeltaEn is enabled */
	uint8_t    TempVoltFallbackIndEn;  /* report when temperature or voltage value back to thresh */
	uint16_t   TempVoltIndPeriod;      /* report peroid when temp or volt keep over thresh, 0 for only report once */
} wlan_ext_temp_volt_thresh_set_t;

typedef enum {
	IEEE80211_MODE_Auto	= 0,	/* autoselect */
	IEEE80211_MODE_11b,			/* 2GHz, CCK */
	IEEE80211_MODE_11g,			/* 2GHz, OFDM */
	IEEE80211_MODE_11ng,		/* 2GHz, w/ HT */
	IEEE80211_MODE_All,
} ieee_phymode;

typedef enum {
	CATEGORY_AC_VO = 0,
	CATEGORY_AC_VI,
	CATEGORY_AC_BE,
	CATEGORY_AC_BK,
} ieeeac_access_category;
/**
 * @brief Parameter for wlan_ext_edca_param
 */
typedef struct wlan_ext_edca_param {
	union {
		ieeeac_access_category type;
		uint32_t type32;
	};
	union {
		ieee_phymode mode;
		uint32_t mode32;
	};
	uint32_t cwmax;
	uint32_t cwmin;
	uint32_t aifsn;
} wlan_ext_edca_param_t;

#define WLAN_EXT_TEMP_THRESH_HIGH_OVERFLOW      (1<<0)
#define WLAN_EXT_TEMP_THRESH_LOW_OVERFLOW       (1<<1)
#define WLAN_EXT_VOLT_THRESH_HIGH_OVERFLOW      (1<<4)
#define WLAN_EXT_VOLT_THRESH_LOW_OVERFLOW       (1<<5)
#define WLAN_EXT_TEMP_VOLT_FALLBACK_TO_THRESH   (1<<11)
typedef struct wlan_ext_temp_volt_event_data {
	uint16_t ind_flags;
	uint16_t reserved;
	int32_t  tmp_now;
	int32_t  tmp_max;
	int32_t  tmp_min;
	uint32_t volt_now;
	uint32_t volt_max;
	uint32_t volt_min;
} wlan_ext_temp_volt_event_data_t;
typedef void (*wlan_ext_temp_volt_event_cb)(wlan_ext_temp_volt_event_data_t *data);
int wlan_ext_set_temp_volt_event_cb(wlan_ext_temp_volt_event_cb cb);
int wlan_ext_temp_volt_event_input(wlan_ext_temp_volt_event_data_t *data);

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_CHANNEL_FEC
 */
typedef struct wlan_ext_channel_fec_set {
	int16_t   FecChannel1;
	int16_t   FecChannel7;
	int16_t   FecChannel13;
	int16_t   Reserved;
} wlan_ext_channel_fec_set_t;

typedef struct wlan_ext_stats_code_get {
	uint16_t    reason_code;
	uint16_t    status_code;
} wlan_ext_stats_code_get_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_AMRR_PARAM
 */
typedef struct wlan_ext_amrr_param {
	uint8_t success_threshold; /* in percent */
	uint8_t failure_threshold; /* in percent */
	int16_t update_threshold;  /* in ms */
} wlan_ext_amrr_param_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_SNIFFER
 */
typedef struct wlan_ext_sniffer_param {
	uint32_t channel;    /* channel 0 means disabling sniffer and switching back to original mode */
	uint32_t dwell_time; /* in us, 0 means disabling sniffer and switching back to original mode */
} wlan_ext_sniffer_param_t;

#define EXT_FLAGS_MCS_RATE   0x1
#define EXT_FLAGS_STBC       0x2
#define EXT_FLAGS_NLTF       0x4
#define EXT_FLAGS_LDPC       0x8
#define EXT_FLAGS_SHORTGI    0x10
#define EXT_FLAGS_BW_40M     0x20
#define EXT_FLAGS_11B_PBCC   0x40

/**
 * @brief Parameter for EXT_FRAME_INFO
 */
typedef struct __ext_frame_info {
	uint8_t    chanNumber;
	uint8_t    rxedRate;
	int8_t     Rssi;
	uint8_t    flags;
	uint16_t   frame_size;
	uint16_t   reserved;
} EXT_FRAME_INFO;

/**
 * @brief Parameter for RX_EXT_FRAMES_IND
 */
typedef struct rx_ext_frames_ind {
	uint16_t  frame_num;  /* pkt num form fw to host*/
	uint16_t  frame_drop; /* pkt num be abandoned by fw, the reason may be that hw buf is not enough */
	EXT_FRAME_INFO frames[1];
} RX_EXT_FRAMES_IND;

#define RX_EXT_FRAME_FILTER_FLAG    0x1
#define RX_EXT_FRAME_FILTER_RATE    0x2
#define RX_EXT_FRAME_FILTER_RSSI    0x4
#define RX_EXT_FRAME_FILTER_SIZE    0x8

/**
 * @brief Parameter for MIB_SET_RX_EXT_FRAME
 */
typedef struct __mib_set_rx_ext_frame_param {
	uint8_t   rx_enable;
	uint8_t   ind_max_num;  /* max num in indication */

	uint8_t   filter_en;    /* filter enable, 0 is no do filter. */

	uint8_t   flags_en;     /* EXT_FLAGS type, valid when RX_EXT_FRAME_FILTER_FLAG is enalbe. */

	/* valid when RX_EXT_FRAME_FILTER_RATE is enalbe. */
	uint8_t   rate_min;
	uint8_t   rate_max;

	/* valid when RX_EXT_FRAME_FILTER_RSSI is enalbe. */
	int8_t   rssi_min;
	int8_t   rssi_max;

	/* valid when RX_EXT_FRAME_FILTER_SIZE is enalbe. */
	uint16_t  size_min;
	uint16_t  size_max;
} MIB_SET_RX_EXT_FRAME;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_LFCLOCK_PARAM
 */
#define SYS_LFCLOCK_TYPE_EXT32K             0
#define SYS_LFCLOCK_TYPE_INT32K_RCOSC       1
#define SYS_LFCLOCK_TYPE_INT32K_RCCAL       2
typedef struct wlan_ext_lfclock_param {
	uint16_t SysLfclockType;
	uint16_t Reserved;
	uint32_t SysLfclockFreq;
	uint32_t PwrCalibCount;
} wlan_ext_lfclock_param_t;

/* for sdd power set/get */
#define SDD_IE_POWER_NUM 11

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_PS_POLICY
 */
typedef struct wlan_ext_ps_policy {
	uint8_t enable;       /* 0: disable ps policy; 1: enable ps policy */
	uint8_t active_flag;  /* bit0: for rx; bit1: for tx */
	uint8_t ps_flag;      /* bit0: for rx; bit1: for tx */
	uint8_t reserve;
	uint16_t active_period;
	uint16_t active_threshold;
	uint16_t ps_period;
	uint16_t ps_threshold;
} wlan_ext_ps_policy_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_PRE_RX_BCN
 */
typedef struct wlan_ext_pre_rx_bcn {
	uint8_t enable;
	uint8_t flags;
	uint8_t stop_num;
	uint8_t reserve;
} wlan_ext_pre_rx_bcn_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_BCN_WITHOUT_DATA
 */
#define IPC_BTH_STATUS_OK                       0x00
#define IPC_BTH_STATUS_BCN_TIM_HOLD             0x01
typedef struct wlan_ext_chk_bcn_without_data_set {
	uint8_t enable;
	uint8_t reserve;
	uint16_t beacon_count;
} wlan_ext_chk_bcn_without_data_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_BCN_TIM_NO_DATA_TMO
 */
typedef struct wlan_ext_bcn_tim_no_data_tmo_set {
	uint8_t enable;
	uint8_t reserve;
	uint16_t timeout_ms;
} wlan_ext_bcn_tim_no_data_tmo_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_GET_BCN_RSSI_DBM
 */
typedef struct wlan_ext_bcn_rssi_dbm_get
{
    int8_t rssi_ewma;  //average rssi value
    int8_t rssi_new;   //last value
} wlan_ext_bcn_rssi_dbm_get_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_BSS_LOSS_THOLD
 */
typedef struct wlan_ext_bss_loss_thold_set {
	uint32_t bss_loss_thold; /* in beacon count */
	uint32_t link_loss_thold; /* in beacon count */
} wlan_ext_bss_loss_thold_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_AUTH/ASSOC_TMO_AND_TRIES
 */
typedef struct wlan_ext_mgmt_timeout_and_tries_set {
	int timeout;
	uint8_t tries;
} wlan_ext_mgmt_timeout_and_tries_set_t;


/**
 * @brief Parameter for WLAN_EXT_CMD_SET_AUTO_POWER
 */
/* adjust power by rules below:
 *    rssi < min_rssi: use default power table value(user power)
 *    min_rssi <= rssi < max_rssi: decrease power in step by every rssi_thres dBm rssi
 *    rssi >= max_rssi: default power - ((max_rssi - min_rssi) / rssi_thres * pwr_step)
 */
typedef struct wlan_ext_auto_power {
	uint16_t period;     /* adjust period, unit in ms */
	uint16_t pwr_step;   /* step of power adjusted every time when the rssi changes rssi_thres dBm, unit in 0.1dBm */
	int16_t rssi_thres;  /* rssi change threshold */
	int16_t max_rssi;    /* max rssi value to stop adjust power */
	int16_t min_rssi;    /* min rssi value to start adjust power */
	uint16_t reserve;
} wlan_ext_auto_power_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_BCN_LOST_COMP
 */
typedef struct wlan_ext_bcn_lost_comp_set {
	uint8_t Enable;
	uint8_t DtimLostNum;  /* num of lost bcn to begin compensate */
	uint8_t CompInterval; /* compensate interval(unit is beacon interval, like 102.4ms) */
	uint8_t CompCnt;      /* compensate count in one DTIM */
} wlan_ext_bcn_lost_comp_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_ARP_KPALIVE
 */
typedef struct wlan_ext_arp_kpalive_set {
	uint16_t ArpKeepAlivePeriod; /* in seconds */
	uint8_t  EncrType; /* ex. WSM_KEY_TYPE_WEP_DEFAULT */
	uint8_t  Reserved;
	uint8_t  SenderIpv4IpAddress[4]; /* in uint32_t big endian format */
	uint8_t  TargetIpv4IpAddress[4];
	uint8_t  TargetMacAddress[6];
} wlan_ext_arp_kpalive_set_t;

/**
 * @brief Parameter for WLAN_EXT_CMD_SET_FILTER_TYPE
 */
#define FILTER_PACKET_BAR    (1 << 0)
#define FILTER_PACKET_PING   (1 << 1)

int wlan_ext_request(struct netif *nif, wlan_ext_cmd_t cmd, uint32_t param);
int wlan_ext_low_power_param_set_default(struct netif *nif, uint32_t dtim);

#ifdef __cplusplus
}
#endif

#endif /* _NET_WLAN_WLAN_EXT_REQ_H_ */
