#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <hal_timer.h>
#include <hal_cmd.h>
#include "sunxi_hal_ledc.h"

#define DEFAULT_BRIGHTNESS	127

void show_default_rgb(void)
{
	printf("======show default RGB======\n");
	sunxi_set_led_brightness(1, DEFAULT_BRIGHTNESS << 8);
	sunxi_set_led_brightness(1, DEFAULT_BRIGHTNESS << 16);
	sunxi_set_led_brightness(1, DEFAULT_BRIGHTNESS << 0);
	sunxi_set_led_brightness(1, 0);
}

int ledc_test_single(int argc, char **argv)
{
	int brightness = 0;
	int led_num;
	int err;

	printf("========SINGLE LEDC TEST========\n");

	err = hal_ledc_init();
	if (err) {
		printf("ledc init error\n");
		return -1;
	}

	if(argc < 3)
	{
		show_default_rgb();
		printf("uasge : hal_ledc [led_num] [RGB] [rgb_brightness], eg: hal_ledc 1 R 100\n");
		return -1;
	}

	led_num = atoi(argv[1]);
	if (led_num < 1 || led_num > 1024)
	{
		printf("The led_num you entered should be between 1 and 1024\n");
	}

	brightness = atoi(argv[3]);

	switch(argv[2][0])
	{
		case 'R' : brightness <<= 8; break;
		case 'G' : brightness <<= 16; break;
		case 'B' : brightness <<= 0; break;
		default  : printf("parameters err\n");
			   return -1;
	}

	err = sunxi_set_led_brightness(led_num, brightness);
	if (err) {
		printf("set all led error\n");
		return -1;
	}

	printf("led is %d\n", led_num);
	printf("brightness is %d\n", brightness);

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(ledc_test_single, hal_ledc, drv single ledc test_code);

int ledc_test_all(int argc, char **argv)
{
	int brightness = 0;
	int led_num;
	int err;

	printf("========ALL LEDC TEST========\n");

	err = hal_ledc_init();
	if (err) {
		printf("ledc init error\n");
		return -1;
	}

	if(argc < 3)
	{
		printf("uasge : hal_ledc_all [led_num] [RGB] [rgb_brightness], eg: hal_ledc_all 34 R 100\n");
		return -1;
	}

	led_num = atoi(argv[1]);
	if (led_num < 1 || led_num > 1024)
	{
		printf("The led_num you entered should be between 1 and 1024\n");
	}

	brightness = atoi(argv[3]);

	switch(argv[2][0])
	{
		case 'R' : brightness <<= 8; break;
		case 'G' : brightness <<= 16; break;
		case 'B' : brightness <<= 0; break;
		default  : printf("parameters err\n");
			   return -1;
	}

	err = sunxi_set_all_led(led_num, brightness);
	if (err) {
		printf("set all led error\n");
		return -1;
	}

	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(ledc_test_all, hal_ledc_all, drv all ledc test_code);
