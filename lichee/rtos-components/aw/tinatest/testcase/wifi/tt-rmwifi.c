#include <tinatest.h>
#include "tt-wifitest.h"

//wifi rmove test
//cmd:tt wrmwifi

int tt_rmwifitest(int argc, char **argv)
{
	int ret;

	printf("\n========TINATEST FOR WIFI DISCONNECT========\n");
	ret = aw_wifi_disconnect();
	if(!ret)
		printf("\n=======TINATEST FOR WIFI DISCONNECT OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI DISCONNECT FAIL=======\n");

	return ret;
}

testcase_init(tt_rmwifitest, wrmwifi, wifi disconnect for tinatest);
