#include <tinatest.h>
#include "tt-wifitest.h"

//wifi connect test
//cmd:tt wcwifi ssid password

int tt_connwifitest(int argc, char **argv)
{
	int ret;
	const char *ssid = "AWTest";
	const char *psw = "1qaz@WSX";
	printf("\n========TINATEST FOR WIFI CONNECT========\n");
	if(argc==2){
		ssid = argv[0];
		psw = argv[1];
	}
	printf("ssid:%s, psw:%s\n", ssid, psw);

	ret = aw_wifi_connect(ssid, psw);
	if(ret)
		goto exit;

	extern void aw_wifi_get_ip_addr(uint32_t *ip);
	ret = -1;
	int wait_s = 20;
	do{
		uint32_t ip = 0;
		aw_wifi_get_ip_addr(&ip);
		if(ip){
			uint8_t ip4[4];
			memcpy(ip4, &ip, 4);
			printf("got ip: %u.%u.%u.%u\n", ip4[0], ip4[1], ip4[2], ip4[3]);
			ret = 0;
			goto exit;
		}
		usleep(1000*1000);
	}while( 0 < wait_s-- );

exit:
	if(!ret)
		printf("\n=======TINATEST FOR WIFI CONNECT OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI CONNECT FAIL=======\n");

	return ret;
}

testcase_init(tt_connwifitest, wcwifi, wifi connect for tinatest);
