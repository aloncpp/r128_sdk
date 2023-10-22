/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 * Email  : liumingyuan@allwinnertech.com
 * Create : 2019-09-12
 *
 */

#ifndef AG_WIFI_IMPL_H
#define AG_WIFI_IMPL_H

#include "AGWifi.h"
#include <iostream>
#include <vector>

#define WIFI_CONFIG "/data/wifi.conf"

class AGWifiMsgListenerImpl : public AGWifiMsgListener
{
private:
	static AGWifiMsgListenerImpl *pInstance;
public:
    virtual ~AGWifiMsgListenerImpl() {}
	static AGWifiMsgListenerImpl* GetInstance() {
		if(NULL == pInstance) {
			pInstance = new AGWifiMsgListenerImpl();
		}
		return pInstance;
	}
    /**
     * @brief Receive wifi message callback
     * @param[in] pWifiMsgData
     * @return void
     */
    void onRecvWifiMsg(WifiMsgData_T *pWifiMsgData);
private:
	void wlan_status(const WifiMsgData_T *pWifiMsgData) const;
	void network_state(const WifiMsgData_T *pWifiMsgData) const;
	void status_trace_from_impl_handle(const WifiMsgData_T *pWifiMsgData) const;
	void network_status_change(const WifiMsgData_T *pWifiMsgData) const;
};

class AGWifiImpl : public AGWifi
{
private:
	static AGWifiImpl *pInstance;


	int save_network_info(const std::string &file_name,const std::string &ctx,std::ios::openmode mode) const ;
	int get_network_info(const std::string &file_name,std::vector<std::string> &ListNetwork);

	int network_format_process(void *raw_info,std::string &netinfo);
public:
	static AGWifiMsgListener *pWifiMsgListener;

    virtual ~AGWifiImpl() {}

	static AGWifiImpl* GetInstance() {
		if(NULL == pInstance) {
			pInstance = new AGWifiImpl();
		}
		return pInstance;
	}
    /**
     * @brief Init AGWifi with listener
     * @param[in] pWifiMsgListener Register listener for wifi message
     * @return void
     */
    void init(AGWifiMsgListener* pWifiMsgListener);
    /**
     * @brief Start AGWifi
     * @return void
     */
    void start();
    /**
     * @brief Get mac for wifi
     * @param[out] mac Output mac address
     * @return int 0 : success; other: fail
     */
    int getMac(char *mac);
	/**
	 * @brief Set mac for wifi
	 * @param[in] mac Intput mac address. mac format is “00E04C871100”
	 * @return int 0 : success; other: fail
	 */
	int setMac(char *mac);

    /**
     * @brief Connect to specific wifi
     * @param[in] ssid
     * @param[in] password
     * @param[in] bssid
     * @param[in] time_ms Timeout value
     * @return int 0 : success; other: fail
     */
    int connect(const char *ssid, const char *password, const char *bssid, int time_ms);
    /**
     * @brief
     * @param[in] result
     * @return void
     */
    void setupResult(int result);
    /**
     * @brief Get Rssi for wifi
     * @param[out] rss Output rssi value
     * @return int 0 : success; other: fail
     */
    int getRssi(int *rssi);
	/**
	 * @brief Get ipaddress
	 * @param[out] ipaddr Output ip addr value
	 * @return int 0 : success; other: fail
	 */
	int getIpAddr(char *ipaddr);
	/**
	 * @brief disconnect from ap
	 * @return int 0 : success; other: fail
	 */
	int disconnect(void);
};
#endif
