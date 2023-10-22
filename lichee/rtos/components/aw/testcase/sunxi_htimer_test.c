/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "../../include/drivers/hal_htimer.h"
#include <stdio.h>
#include <tinatest.h>

void timer_call_back_test(void *param)
{
	printf("timer test!\n");
}

int htimer_oneshot_test(int argc, char **argv)
{
	int ret;
	ret = hal_htimer_set_oneshot(HAL_HRTIMER0, 100, timer_call_back_test, NULL);

	if(ret < 0) {
		printf("set oneshot error!\n");
		return ret;
	}

	return ret;
}

int htimer_periodic_test(int argc, char **argv)
{
	int ret = 0;
	ret = hal_htimer_set_periodic(HAL_HRTIMER0 ,200, timer_call_back_test, NULL);

	if(ret < 0) {
		printf("set periodic error!\n");
		return ret;
	}

	return ret;
}

int htimer_start(int argc, char **argv)
{
	//hal_htimer_start(HAL_HRTIMER0);
	return 0;
}

int htimer_stop(int argc, char **argv)
{
	hal_htimer_stop(HAL_HRTIMER0);
	return 0;
}

tt_funclist_t tt_htimertest_funclist[] = {
	{"htimer oneshot test", htimer_oneshot_test},
	{"htimer periodic test", htimer_periodic_test},
	{"htimer start", htimer_start},
	{"htimer stop", htimer_stop},
};

testcase_init_with_funclist(tt_htimertest_funclist, htimer_test,testcase funclist test);
