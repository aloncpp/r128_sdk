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

#ifndef _NET_WLAN_WLAN_FRAME_H_
#define _NET_WLAN_WLAN_FRAME_H_

#include "net/wlan/wlan_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t wlan_construct_beacon(uint8_t *frm, uint32_t len,
                               const uint8_t sa[IEEE80211_ADDR_LEN],
                               const uint8_t da[IEEE80211_ADDR_LEN],
                               const uint8_t bssid[IEEE80211_ADDR_LEN],
                               const uint8_t *ssid, size_t ssidlen,
                               uint8_t ch);
uint32_t wlan_construct_probereq(uint8_t *frm, uint32_t len,
                                 const uint8_t sa[IEEE80211_ADDR_LEN],
                                 const uint8_t da[IEEE80211_ADDR_LEN],
                                 const uint8_t bssid[IEEE80211_ADDR_LEN],
                                 const uint8_t *ssid, size_t ssidlen);
uint32_t wlan_construct_probersp(uint8_t *frm, uint32_t len,
                                 const uint8_t sa[IEEE80211_ADDR_LEN],
                                 const uint8_t da[IEEE80211_ADDR_LEN],
                                 const uint8_t bssid[IEEE80211_ADDR_LEN],
                                 const uint8_t *ssid, size_t ssidlen,
                                 uint8_t ch);
uint32_t wlan_construct_assocreq(uint8_t *frm, uint32_t len,
                                 const uint8_t sa[IEEE80211_ADDR_LEN],
                                 const uint8_t da[IEEE80211_ADDR_LEN],
                                 const uint8_t bssid[IEEE80211_ADDR_LEN],
                                 const uint8_t *ssid, size_t ssidlen);
uint32_t wlan_construct_assocrsp(uint8_t *frm, uint32_t len,
                                 const uint8_t sa[IEEE80211_ADDR_LEN],
                                 const uint8_t da[IEEE80211_ADDR_LEN],
                                 const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_reassocreq(uint8_t *frm, uint32_t len,
                                   const uint8_t sa[IEEE80211_ADDR_LEN],
                                   const uint8_t da[IEEE80211_ADDR_LEN],
                                   const uint8_t bssid[IEEE80211_ADDR_LEN],
                                   const uint8_t *ssid, size_t ssidlen);
uint32_t wlan_construct_reassocrsp(uint8_t *frm, uint32_t len,
                                   const uint8_t sa[IEEE80211_ADDR_LEN],
                                   const uint8_t da[IEEE80211_ADDR_LEN],
                                   const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_disassocreq(uint8_t *frm, uint32_t len,
                                    const uint8_t sa[IEEE80211_ADDR_LEN],
                                    const uint8_t da[IEEE80211_ADDR_LEN],
                                    const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_atim(uint8_t *frm, uint32_t len,
                             const uint8_t sa[IEEE80211_ADDR_LEN],
                             const uint8_t da[IEEE80211_ADDR_LEN],
                             const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_auth(uint8_t *frm, uint32_t len,
                             const uint8_t sa[IEEE80211_ADDR_LEN],
                             const uint8_t da[IEEE80211_ADDR_LEN],
                             const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_deauth(uint8_t *frm, uint32_t len,
                               const uint8_t sa[IEEE80211_ADDR_LEN],
                               const uint8_t da[IEEE80211_ADDR_LEN],
                               const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_action(uint8_t *frm, uint32_t len,
                               const uint8_t sa[IEEE80211_ADDR_LEN],
                               const uint8_t da[IEEE80211_ADDR_LEN],
                               const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_sa_query(uint8_t *frm, uint32_t len,
                                 const uint8_t sa[IEEE80211_ADDR_LEN],
                                 const uint8_t da[IEEE80211_ADDR_LEN],
                                 const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_nulldata(uint8_t *frm, uint32_t len,
                                 const uint8_t sa[IEEE80211_ADDR_LEN],
                                 const uint8_t da[IEEE80211_ADDR_LEN],
                                 const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_data(uint8_t *frm, uint32_t len,
                             const uint8_t sa[IEEE80211_ADDR_LEN],
                             const uint8_t da[IEEE80211_ADDR_LEN],
                             const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_qosdata(uint8_t *frm, uint32_t len,
                                const uint8_t sa[IEEE80211_ADDR_LEN],
                                const uint8_t da[IEEE80211_ADDR_LEN],
                                const uint8_t bssid[IEEE80211_ADDR_LEN]);
uint32_t wlan_construct_arpreq(uint8_t *frm, uint32_t len,
                               const uint8_t sa[IEEE80211_ADDR_LEN],
                               const uint8_t da[IEEE80211_ADDR_LEN],
                               const uint8_t bssid[IEEE80211_ADDR_LEN],
                               const uint8_t *src_ip);

#ifdef __cplusplus
}
#endif

#endif /* _NET_WLAN_WLAN_FRAME_H_ */