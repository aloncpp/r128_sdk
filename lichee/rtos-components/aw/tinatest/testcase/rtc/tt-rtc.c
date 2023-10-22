#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <pm_state.h>
#include <pm_base.h>

#include <tinatest.h>
#include <sunxi_hal_rtc.h>

#include <barrier.h>

static int callback(void)
{
	printf("alarm interrupt\n");
	return 0;
}

extern int pm_trigger_suspend(suspend_mode_t mode); 
//tt rtctester
int tt_rtctest(int argc, char **argv)
{
    int ret = 0;
	unsigned int enable = 1;
	struct rtc_time rtc_tm;
	struct rtc_wkalrm wkalrm;

	hal_rtc_init();
	hal_rtc_register_callback(callback);

	if (hal_rtc_gettime(&rtc_tm))
		printf("sunxi rtc gettime error\n");

	wkalrm.enabled = 1;
	wkalrm.time = rtc_tm;
	if(rtc_tm.tm_sec > 5)
		 rtc_tm.tm_sec -= 5;
	else
		wkalrm.time.tm_sec += 5;

	if (hal_rtc_settime(&rtc_tm))
		printf("sunxi rtc settime error\n");

	if (hal_rtc_setalarm(&wkalrm))
		printf("sunxi rtc setalarm error\n");

	if (hal_rtc_getalarm(&wkalrm))
		printf("sunxi rtc getalarm error\n");

	if (hal_rtc_gettime(&rtc_tm))
		printf("sunxi rtc gettime error\n");

	//if do hal_rtc_alarm_irq_enable and hal_rtc_uninit, alarm will not work
	 hal_rtc_alarm_irq_enable(enable);



	ret = pm_trigger_suspend(PM_MODE_STANDBY);
	if (!ret)
		printf("===== %s: trigger suspend ok, return %d =====\n", __func__, ret);
	else {
		printf("===== %s: trigger suspend fail, return %d =====\n", __func__, ret);
		return -1;
	}


	printf("====rtctest successful!====\n");

	return 0;

}

testcase_init(tt_rtctest, rtctester, rtctester for tinatest);
