/*
 * Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
 * Email  : liumingyuan@allwinnertech.com
 * Create : 2019-09-12
 *
 */

#include "AGWifi.h"
#include "AGWifiImpl.h"
#include "AGWifiLog.h"
#include "wifi_glue.h"
#include <cstring>
#include <fstream>
static const char* wifi_status_impl_to_str(int sta)
{
	switch(sta) {
		case GLWIFI_DISCONNECTED:
			return "WIFI_DISCONNECTED";
		case GLWIFI_SCAN_STARTED:
			return "WIFI_SCAN_STARTED";
		case GLWIFI_SCAN_FAILED:
			return "WIFI_SCAN_FAILED";
		case GLWIFI_NETWORK_NOT_FOUND:
			return "WIFI_NETWORK_NOT_FOUND";
		case GLWIFI_ASSOCIATING:
			return "WIFI_ASSOCIATING";
		case GLWIFI_AUTH_REJECT:
			return "WIFI_AUTH_REJECT";
		case GLWIFI_AUTH_TIMEOUT:
			return "WIFI_AUTH_TIMEOUT";
		case GLWIFI_HANDSHAKE_FAILED:
			return "WIFI_HANDSHAKE_FAILED";
		case GLWIFI_CONNECTED:
			return "WIFI_CONNECTED";
		case GLWIFI_CONN_TIMEOUT:
			return "WIFI_CONN_TIMEOUT";
		case GLDHCP_START_FAILED:
			return "DHCP_START_FAILED";
		case GLDHCP_TIMEOUT:
			return "DHCP_TIMEOUT";
		case GLDHCP_SUCCESS:
			return "DHCP_SUCCESS";
		default:
			return "UNKNOWN";
	}
}
void AGWifiMsgListenerImpl::wlan_status(const WifiMsgData_T *pWifiMsgData) const
{
	WFLOGD("wlan status : %s",(pWifiMsgData->data.wlanStatus == WLAN0_STATUS_UP) ?
			"up":"down");
}
void AGWifiMsgListenerImpl::network_state(const WifiMsgData_T *pWifiMsgData) const
{
	WFLOGD("wifi state:%s",wifi_status_impl_to_str(pWifiMsgData->data.wlanStatus));
}
void AGWifiMsgListenerImpl::status_trace_from_impl_handle(const WifiMsgData_T *pWifiMsgData) const
{
	WFLOGD("enter -+-");
}
void AGWifiMsgListenerImpl::network_status_change(const WifiMsgData_T *pWifiMsgData) const
{
	GLWifiStaChange_t *pSta = (GLWifiStaChange_t*)pWifiMsgData->data.networkStatusChange;
	WFLOGD("ssid:%s,password:%s,status:%s",pSta->ssid,
			pSta->password,pSta->status);
}


void AGWifiMsgListenerImpl::onRecvWifiMsg(WifiMsgData_T *pWifiMsgData)
{
	if(NULL == pWifiMsgData) {
		WFLOGE("WifiMsgData is NULL.");
		return;
	}
	switch(pWifiMsgData->id) {
		case WIFI_MSG_ID_WIFI_STATUS:
			wlan_status(pWifiMsgData);
			break;
		case WIFI_MSG_ID_WIFI_STATUS_FROM_IMPL:
			network_state(pWifiMsgData);
			break;
		case WIFI_MSG_ID_WIFI_TRACE_FROM_IMPL:
			status_trace_from_impl_handle(pWifiMsgData);
			break;
		case WIFI_MSG_ID_NETWORK_STATUS:
			network_status_change(pWifiMsgData);
			break;
		default:
			WFLOGI("Not implemented:%d",pWifiMsgData->id);
			break;
	}
}

AGWifiMsgListener* AGWifiImpl::pWifiMsgListener = NULL;

static void AGWifiMsgInd(GLWifiMsgData_t *pMsgData)
{
	WifiMsgData_T WifiMsgData;

	memcpy(&WifiMsgData,pMsgData,sizeof(GLWifiMsgData_t));

	if(AGWifiImpl::pWifiMsgListener) {
		AGWifiImpl::pWifiMsgListener->onRecvWifiMsg(&WifiMsgData);
	}
}



/**
 * @brief Init AGWifi with listener
 * @param[in] pWifiMsgListener Register listener for wifi message
 * @return void
 */
void AGWifiImpl::init(AGWifiMsgListener* pWifiMsgListener)
{
	this->pWifiMsgListener = pWifiMsgListener;
	glue_set_wifi_msg_cb(AGWifiMsgInd);
}
/**
 * @brief Start AGWifi
 * @return void
 */
void AGWifiImpl::start()
{
	glue_wifi_on();
}
/**
 * @brief Get mac for wifi
 * @param[out] mac Output mac address
 * @return int 0 : success; other: fail
 */
int AGWifiImpl::getMac(char *mac)
{
	return glue_get_mac(mac);
}
/**
 * @brief Set mac for wifi
 * @param[in] mac Intput mac address
 * @return int 0 : success; other: fail
 */
int AGWifiImpl::setMac(char *mac)
{
	return glue_set_mac(mac);
}
/**
 * @brief Connect to specific wifi
 * @param[in] ssid
 * @param[in] password
 * @param[in] bssid
 * @param[in] time_ms Timeout value
 * @return int 0 : success; other: fail
 */
int AGWifiImpl::connect(const char *ssid, const char *password, const char *bssid, int time_ms)
{
	return glue_wifi_connect(ssid,password,bssid,time_ms);
}

/**
 * @brief disconnect from ap
 * @return int 0 : success; other: fail
 */

int AGWifiImpl::disconnect(void)
{
	return glue_wifi_disconnect();
}

int AGWifiImpl::network_format_process(void *raw_info,std::string &netinfo)
{
	GLWifiConfig_t *ptr_net = (GLWifiConfig_t *)raw_info;

	netinfo.append("network={");
	netinfo.append("\r\n");
	netinfo.append("\tssid=");
	netinfo.append((const char*)ptr_net->ssid,strlen(ptr_net->ssid));
	netinfo.append("\r\n");
	netinfo.append("\tpassword=");
	netinfo.append((const char*)ptr_net->pwd,strlen(ptr_net->pwd));
	netinfo.append("\r\n");
	netinfo.append("\tbssid=");
	netinfo.append(mac_to_str(ptr_net->bssid));
	netinfo.append("\r\n");
	netinfo.append("\tchannel=");
	netinfo.append(std::to_string(ptr_net->ch));
	netinfo.append("\r\n");
	netinfo.append("\taddr=");
	netinfo.append(mac_to_str(ptr_net->addr));
	netinfo.append("\r\n");
	netinfo.append("\tip_addr=");
	netinfo.append(ip_to_str(ptr_net->ip_addr));
	netinfo.append("\r\n");
	netinfo.append("\twpa_state=connected");
	netinfo.append("\r\n");
	netinfo.append("}");
	netinfo.append("\r\n");
	return 0;
}

int AGWifiImpl::save_network_info(const std::string &file_name,const std::string &ctx,std::ios::openmode mode) const
{
	std::ofstream OsWrite(file_name,mode);
	if(OsWrite.is_open()) {
		OsWrite << ctx;
		OsWrite.close();
	}else
		return -1;
	return 0;
}

int AGWifiImpl::get_network_info(const std::string &file_name,
		std::vector<std::string> &ListNetwork)
{
	int ret = 0;
	std::ifstream infile;
	std::string SingleNetInfo;

	infile.open(file_name,std::ios::in);
	if(!infile) {
		WFLOGD("%s file is not exist.",file_name.c_str());
		ret = -1;
	}else {
		while(!infile.eof()) {
			std::string tmp;
			WFLOGD("find network info start.");
#if 1
			while(getline(infile,tmp)) {
				WFLOGD("%d,%s",tmp.length(),tmp.c_str());
				SingleNetInfo.append(tmp.c_str(),tmp.length());
				SingleNetInfo.append("\r\n");
				std::size_t found = tmp.find("}");
				if(found != std::string::npos) {
					WFLOGD("find network info end.");
					break;
				}
			}
#else
			char c;
			while(!infile.eof()) {
				infile >> c;
				SingleNetInfo.push_back(c);
				SingleNetInfo.append("\r\n");
				if(c == '}')
					break;
			}
#endif
			if(!infile.eof()) {
				ListNetwork.push_back(SingleNetInfo);
			}
			SingleNetInfo.clear();
		}
	}
	infile.close();
	return ret;
}
/**
 * @brief
 * @param[in] result
 * @return void
 */
void AGWifiImpl::setupResult(int result)
{
	GLWifiConfig_t raw_config;

	std::vector<std::string> ListNetwork;
	std::string Network;
	int ret;

	if(!result) {
		ret = glue_get_wifi_config(&raw_config);
		if(ret == -1) {
			WFLOGE("get wifi raw config failed.");
			return ;
		}

		network_format_process((void*)&raw_config,Network);
		WFLOGD("network info : %s",Network.c_str());

		ret = get_network_info(std::string(WIFI_CONFIG),ListNetwork);
		if(ret == -1) {
			save_network_info(std::string(WIFI_CONFIG),Network,std::ios::out);
		}else{

			WFLOGD("net info num:%d",ListNetwork.size());
			std::vector<std::string>::iterator it = ListNetwork.begin();
			while(it !=ListNetwork.end()) {
				std::size_t found = it->find(raw_config.ssid,strlen(raw_config.ssid));
				if(found != std::string::npos) {
					WFLOGD("found the same ssid : %s",raw_config.ssid);
					break;
				}
				it++;
			}
			if(it != ListNetwork.end()) {
				WFLOGD("remove same ssid element:%s",raw_config.ssid);
				ListNetwork.erase(it);
			}

			if(ListNetwork.size() >= 3) {
				WFLOGD("remove first netinfo :%s",ListNetwork.begin()->c_str());
				ListNetwork.erase(ListNetwork.begin());
			}

			ListNetwork.push_back(Network);
			WFLOGD("net info num:%d",ListNetwork.size());

			for(std::vector<std::string>::iterator it = ListNetwork.begin();it != ListNetwork.end(); ++it) {
				WFLOGD("save net info %s",it->c_str());
				if(it == ListNetwork.begin()){
					save_network_info(std::string(WIFI_CONFIG),*it,std::ios::out);
				} else {
					save_network_info(std::string(WIFI_CONFIG),*it,std::ios::app);
				}
			}
		}
	}else {
		ret = get_network_info(std::string(WIFI_CONFIG),ListNetwork);
		if(ret == -1)
			return ;
		WFLOGD("net info num:%d",ListNetwork.size());
		std::vector<std::string>::iterator it = ListNetwork.begin();
		while(it != ListNetwork.end()) {

			std::string ssid;
			std::string password;
			int i = 0;
			char c;

			std::size_t found = it->find("ssid=",5);
			if(found != std::string::npos) {
				while((c= (*it)[found+5+i++]) != '\r') {
					ssid.push_back(c);
				}
				ssid.push_back('\0');
				WFLOGD("ssid : %s",ssid.c_str());
			}else {
				WFLOGE("Can't find ssid.");
				return ;
			}

			i = 0;

			found = it->find("password=",9);
			if(found != std::string::npos) {
				while((c= (*it)[found+9+i++]) != '\r') {
					password.push_back(c);
				}
				password.push_back('\0');
				WFLOGD("password : %s",password.c_str());
			}else {
				WFLOGE("Can't find password.");
				return ;
			}


			int ret ;
			ret = connect(ssid.c_str(),password.c_str(),NULL,0);
			if(ret == 0) {
				break;
			}
			it++;
		}
	}
}
/**
 * @brief Get Rssi for wifi
 * @param[out] rss Output rssi value
 * @return int 0 : success; other: fail
 */
int AGWifiImpl::getRssi(int *rssi)
{
	return glue_get_rssi(rssi);
}
/**
 * @brief Get ipaddress
 * @param[out] ipaddr Output ip addr value
 * @return int 0 : success; other: fail
 */
int AGWifiImpl::getIpAddr(char *ipaddr)
{
	return glue_get_ipaddr(ipaddr);
}
