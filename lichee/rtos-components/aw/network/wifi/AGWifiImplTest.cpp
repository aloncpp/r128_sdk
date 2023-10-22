#include <console.h>
#include <string.h>
#include <unistd.h>
#include "AGWifi.h"
#include "AGWifiImpl.h"
#include "AGWifiLog.h"
#include "wifi_glue.h"

AGWifiImpl* AGWifiImpl::pInstance = NULL;
AGWifiMsgListenerImpl* AGWifiMsgListenerImpl::pInstance = NULL;

static void usage(void)
{
	WFLOGI("Usage:agwifi [option] args......");
	WFLOGI("-c <ssid> <password> connect to ap.");
	WFLOGI("-d disconnect.");
	WFLOGI("-s <mac> set mac address.");
	WFLOGI("-g <mac/rssi/ipaddr>");
	WFLOGI("-r get history netinfo from file to connect ap.");
}

void AGWifiTask(int argc, char **argv)
{
	int c;

	AGWifi *pAGWifiHd = AGWifiImpl::GetInstance();

	AGWifiMsgListenerImpl* pAGWifiMsgListener = AGWifiMsgListenerImpl::GetInstance();

	pAGWifiHd->init(pAGWifiMsgListener);

	pAGWifiHd->start();

	optind = 0;
	while((c = getopt(argc,argv,"c:s:ghdr")) != -1) {
		switch(c) {
			case 'c':
				if((argc != 3) && (argc != 4) && (argc != 5)) {
					goto usage;
				}
				if(pAGWifiHd->connect(argv[2],argv[3],NULL,0) != 0) {
					WFLOGI("connect failed.");
				}else{
					WFLOGI("connect success.");
					pAGWifiHd->setupResult(0);
				}
				break;
			case 'r':
				if(argc != 2) {
					goto usage;
				}
				pAGWifiHd->setupResult(1); // get ssid and password from file to connect.
				break;
			case 's':
				if(argc != 3) {
					goto usage;
				}
				if(dynamic_cast<AGWifiImpl*>(pAGWifiHd)->setMac(argv[2]) != 0) {
					WFLOGE("set mac failed.");
				}
				break;
			case 'd':
				if(argc != 2) {
					goto usage;
				}
				if(dynamic_cast<AGWifiImpl*>(pAGWifiHd)->disconnect() != 0) {
					WFLOGE("disconnect failed.");
				}
				break;
			case 'g':
				if(argc != 3) {
					goto usage;
				}
				if(strstr(argv[2],"mac")) {
					char mac[18] = {0};
					pAGWifiHd->getMac(mac);
					WFLOGI("mac:%s",mac);
				}else if(strstr(argv[2],"rssi")) {
					int rssi = 0;
					pAGWifiHd->getRssi(&rssi);
					WFLOGI("rssi:%d",rssi);
				}else if(strstr(argv[2],"ipaddr")) {
					char ipaddr[16];
					dynamic_cast<AGWifiImpl*>(pAGWifiHd)->getIpAddr(ipaddr);
					WFLOGI("ipaddress:%s",ipaddr);
				}else {
					goto usage;
				}
				break;
			case 'h':
usage:
				usage();
				return;
		}
	}

}
FINSH_FUNCTION_EXPORT_CMD(AGWifiTask,agwifi, Console aligenie wifi test Command);
