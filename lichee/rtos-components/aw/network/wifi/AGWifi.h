/**
 * Copyright (C) 2018 Alibaba.inc, All rights reserved.
 *
 * @file AGWifi.h
 * @brief
 * @author zhibin.yzb@alibaba-inc.com
 * @data 2018/03/09
 * @version 1.0
 */

#ifndef AG_WIFI_H
#define AG_WIFI_H

typedef enum
{
    WLAN0_STATUS_DOWN,
    WLAN0_STATUS_UP,
    WLAN0_STATUS_MAX
} WifiStatus_E;

typedef enum
{
    WIFI_MSG_ID_MIN = 0,
    WIFI_MSG_ID_WIFI_STATUS = WIFI_MSG_ID_MIN,
    WIFI_MSG_ID_WIFI_STATUS_FROM_IMPL,
    WIFI_MSG_ID_WIFI_TRACE_FROM_IMPL,
    WIFI_MSG_ID_NETWORK_STATUS,
    WIFI_MSG_ID_MAX
} WifiMsgId_E;

typedef struct
{
    int id;
    union {
        int   wlanStatus;
        void *networkStatusChange;
        void *wifiTrace;
    } data;
} WifiMsgData_T;

class AGWifiMsgListener
{
public:
    virtual ~AGWifiMsgListener() {}
    /**
     * @brief Receive wifi message callback
     * @param[in] pWifiMsgData
     * @return void
     */
    virtual void onRecvWifiMsg(WifiMsgData_T *pWifiMsgData) = 0;
};

class AGWifi
{
public:
    virtual ~AGWifi() {}
    /**
     * @brief Init AGWifi with listener
     * @param[in] pWifiMsgListener Register listener for wifi message
     * @return void
     */
    virtual void init(AGWifiMsgListener* pWifiMsgListener) = 0;
    /**
     * @brief Start AGWifi
     * @return void
     */
    virtual void start() = 0;
    /**
     * @brief Get mac for wifi
     * @param[out] mac Output mac address
     * @return int 0 : success; other: fail
     */
    virtual int getMac(char *mac)=0;
    /**
     * @brief Connect to specific wifi
     * @param[in] ssid
     * @param[in] password
     * @param[in] bssid
     * @param[in] time_ms Timeout value
     * @return int 0 : success; other: fail
     */
    virtual int connect(const char *ssid, const char *password, const char *bssid, int time_ms)=0;
    /**
     * @brief
     * @param[in] result
     * @return void
     */
    virtual void setupResult(int result)=0;
    /**
     * @brief Get Rssi for wifi
     * @param[out] rss Output rssi value
     * @return int 0 : success; other: fail
     */
    virtual int getRssi(int *rssi)=0;
};
#endif
