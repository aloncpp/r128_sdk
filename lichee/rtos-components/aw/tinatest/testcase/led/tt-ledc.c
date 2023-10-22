#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>
#include <tinatest.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../../../../../drivers/drv/leds/leds-sunxi.h"

#define DEFAULT_BRIGHTNESS	127
void show_default_rgb()
{
	printf("======show default RGB======\n");

	sunxi_set_leds_brightness(1, DEFAULT_BRIGHTNESS << 8);
	vTaskDelay(1000/portTICK_RATE_MS);

	sunxi_set_leds_brightness(1, DEFAULT_BRIGHTNESS << 16);
	vTaskDelay(1000/portTICK_RATE_MS);

	sunxi_set_leds_brightness(1, DEFAULT_BRIGHTNESS << 0);
	vTaskDelay(1000/portTICK_RATE_MS);

	sunxi_set_leds_brightness(1, 0);

}

//tt ledctest
//or : tt ledctest R 100(set led show red, brightness : 100)
int tt_ledctest(int argc, char **argv)
{
	int brightness = 0;

	printf("========LEDC TEST========\n");

	sunxi_leds_init();

	if(argc < 3)
	{
		show_default_rgb();
		return 0;
	}

	brightness = atoi(argv[2]);

	switch(argv[1][0])
	{
		case 'R' : brightness <<= 8; break;
		case 'G' : brightness <<= 16; break;
		case 'B' : brightness <<= 0; break;
		default  : "parameters err\n";
			   return -1;
	}
	sunxi_set_led_brightness(1, brightness);
	vTaskDelay(1000/portTICK_RATE_MS);
	sunxi_set_led_brightness(1, 0);

	return 0;
}
testcase_init(tt_ledctest, ledctest, ledctest for tinatest);
