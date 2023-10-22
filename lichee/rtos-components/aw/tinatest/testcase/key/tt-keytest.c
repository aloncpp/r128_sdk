#include <FreeRTOS.h>
#include <stdint.h>
#include <tinatest.h>
#include <stdio.h>
#include "sunxi-input.h"

#if defined(CONFIG_DRIVERS_GPADC_KEY)
#include "../../../../../../drivers/drv/input/keyboard/gpadc-key.h"
#define DEVICE_NAME "gpadc-key"
#define KEY1 KEY_VOLUMEUP
#define KEY2 KEY_VOLUMEDOWN
#define KEY3 KEY_MENU
#define KEY4 KEY_PLAYPAUSE
#define KEY5 KEY_POWER
#define NUM_OF_KEYS 4
#endif

#if defined(CONFIG_DRIVERS_SUNXI_KEYBOARD)
#include "../../../../../../drivers/drv/input/keyboard/sunxi-keyboard.h"
#define DEVICE_NAME "sunxi-keyboard"
#define KEY1 KEY_VOLUMEUP
#define KEY2 KEY_VOLUMEDOWN
#define KEY3 KEY_POWER
#define KEY4 KEY_PLAYPAUSE
#define KEY5 KEY_MICMUTE
#define NUM_OF_KEYS 5
#endif

int tt_keytest(int argc, char **argv)
{
    int i;
    int key_count = 0;
    int fd = -1;
    int press_key_code[NUM_OF_KEYS] = {0};
    struct sunxi_input_event event;

    printf("========Now is %s test!========\n", DEVICE_NAME);
	
#if defined(CONFIG_DRIVERS_GPADC_KEY)
	sunxi_gpadc_key_init();
#endif

#if defined(CONFIG_DRIVERS_SUNXI_KEYBOARD)
    sunxi_keyboard_init();
#endif
    fd = sunxi_input_open(DEVICE_NAME);
    if (fd < 0) {
	    printf("====keyboard open err====\n");
	    return -1;
    }

	while (key_count < NUM_OF_KEYS) {
		sunxi_input_readb(fd, &event, sizeof(struct sunxi_input_event));

		if (event.type != EV_KEY)
			continue;
		if (event.value == 0){
			printf("keyup!\n");
			continue;
		}

	    switch (event.code) {
		    case KEY1 :
			    printf("KEY1(%d) press\n", KEY1);
			    break;
		    case KEY2 :
			    printf("KEY2(%d) press\n", KEY2);
			    break;
		    case KEY3 :
			    printf("KEY3(%d) press\n", KEY3);
			    break;
		    case KEY4 :
			    printf("KEY4(%d) press\n", KEY4);
			    break;
		    case KEY5 :
			    printf("KEY5(%d) press\n", KEY5);
			    break;

		    default :
			    printf("unknow key press\n");
			    break;
	    }

	    for (i = 0; i < NUM_OF_KEYS; i++) {
		    if (press_key_code[i] == event.code)
			    break;
	    }

	    if (i == NUM_OF_KEYS)
		    press_key_code[key_count++] = event.code;
    }

    printf("all keys press\n");

    return 0;
}
testcase_init(tt_keytest, keytest, keytest for tinatest);
