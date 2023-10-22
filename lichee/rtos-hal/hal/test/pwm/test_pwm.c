/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY'S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS'SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY'S TECHNOLOGY.
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <sunxi_hal_pwm.h>

#if defined(CONFIG_ARCH_DSP)
#include <delay.h>
#define sleep(sec) msleep(sec * 1000)
#endif

#ifndef printfFromISR
#define printfFromISR printf
#endif

static int cmd_test_pwm(int argc, char **argv)
{

    struct pwm_config *config;
    uint8_t port;
    //uint8_t ns;
    int period, duty;

    if (argc < 4)
    {
        hal_log_info("Usage: pwm port | duty | period\n");
        return -1;
    }

    hal_log_info("Run pwm hal layer test case\n");

    port = strtol(argv[1], NULL, 0);
    hal_log_info("port = %d", port);
    duty = strtoul(argv[2], NULL, 0);
    period = strtoul(argv[3], NULL, 0);

    config = (struct pwm_config *)malloc(sizeof(struct pwm_config));

    config->duty_ns   = duty;
    config->period_ns = period;
    config->polarity  = PWM_POLARITY_NORMAL;
    hal_log_info("duty_ns = %d \n", config->duty_ns);
    hal_log_info("period_ns = %d \n", config->period_ns);
    hal_log_info("polarity = %d \n", config->polarity);

    hal_pwm_init();

    hal_pwm_control(port, config);

    hal_log_info("control pwm test finish\n");

    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_pwm, hal_pwm, pwm hal APIs tests)

static int cmd_release_pwm_channel(int argc, char **argv)
{

    struct pwm_config *config;
    uint8_t port;
    uint8_t ns;
    ulong period, duty;

    if (argc < 2)
    {
        hal_log_info("Usage: pwm port\n");
        return -1;
    }

    hal_log_info("Run close pwm channel test case\n");

    port = strtol(argv[1], NULL, 0);
    hal_log_info("port = %d", port);

    hal_pwm_release(port);

    hal_log_info("release pwm channel finish\n");

    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_release_pwm_channel, hal_pwm_close, release pwm channel hal APIs tests)

static void pwm_cap_callback(void* param)
{
	hal_pwm_cap_info *info = (hal_pwm_cap_info *)param;

	printfFromISR("pwm%d capture callback, cnt is %d, period is %d, duty is %d\n", info->channel, info->cnt, info->period, info->duty);
}

hal_pwm_status_t pwm_capture_init(uint32_t channel)
{

	hal_pwm_cap_enable(channel, pwm_cap_callback);

	return HAL_PWM_STATUS_OK;
}

hal_pwm_status_t pwm_capture_deinit(uint32_t channel)
{
	hal_pwm_cap_disable(channel);

	return HAL_PWM_STATUS_OK;
}

void cmd_pwm_capture_help(void)
{
	printf("pwm capture test, stop capture after 10s\n");
	printf("usage: pwm_capture_test<channel>\n");
	printf("\t<channel>: 0 ~ 15\n");
	printf("eg: pwm_capture_test 0, pwm0 capture function\n");
}

int cmd_pwm_capture(int argc, char *argv[])
{
	int count = 0;

	if (argc != 2 || atoi(argv[1]) > 15) {
		cmd_pwm_capture_help();
		return -1;
	}

	/* capture setting */
	pwm_capture_init(atoi(argv[1]));

	for(count = 0; count < 10; count++) {
		sleep(1);
	}

	/* disable */
	pwm_capture_deinit(atoi(argv[1]));

	printf("[%s]: pwm stop capture ssd\n", __func__);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_pwm_capture, pwm_capture_test, pwm capture test)
