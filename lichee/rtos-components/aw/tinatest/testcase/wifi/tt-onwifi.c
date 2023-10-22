#include <tinatest.h>
#include "tt-wifitest.h"

//wifi on test
//cmd:tt wonwifi
int tt_onwifitest(int argc, char **argv)
{
	int ret;

	printf("\n========TINATEST FOR WIFI ON========\n");
	ret = aw_wifi_on(WIFI_MODE_STA);
	if(!ret)
		printf("\n=======TINATEST FOR WIFI ON OK=======\n");
	else
		printf("\n=======TINATEST FOR WIFI ON FAIL=======\n");

	return ret;
}
testcase_init(tt_onwifitest, wonwifi, wifi on for tinatest);
