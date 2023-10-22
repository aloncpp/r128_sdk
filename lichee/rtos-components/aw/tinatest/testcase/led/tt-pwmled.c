#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <tinatest.h>
#include <stdio.h>

#include "../../../../../../drivers/drv/leds/pwm-leds.h"

int tt_pwmtest(int argc, char **argv)
{
    int ret = -1;

    printf("=========PWM TEST========\n");
    ret = sunxi_pwm_led_init();
    if (ret < 0) {
	    printf("sunxi_pwm_led_init fail\n");

	    return -1;
    }

    sunxi_set_pwm_led_brightness(0x0000FF);
    vTaskDelay(1000/portTICK_RATE_MS);

    sunxi_set_pwm_led_brightness(0x00FF00);
    vTaskDelay(1000/portTICK_RATE_MS);

    sunxi_set_pwm_led_brightness(0xFF0000);
    vTaskDelay(1000/portTICK_RATE_MS);

    sunxi_set_pwm_led_brightness(0x000000);

    return 0;
}
testcase_init(tt_pwmtest, pwmtest, pwmtest for tinatest);
