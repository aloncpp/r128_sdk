#include <tinatest.h>
#include "tt-wifitest.h"

//wifi off test
//cmd:tt woffwifi

int tt_offwifitest(int argc, char **argv)
{
	int ret = 0;

	printf("\n========TINATEST FOR WIFI OFF========\n");
	aw_wifi_off();
	printf("\n========TINATEST FOR WIFI OFF OK========\n");

	return ret;
}

testcase_init(tt_offwifitest, woffwifi, wifi off for tinatest);
