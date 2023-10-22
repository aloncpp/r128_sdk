#ifndef __GPIO_KEY_POLLED_H
#define __GPIO_KEY_POLLED_H

#define ACTIVE_LOW	0
#define ACTIVE_HIGH	1

//gpio key config for test
struct gpio_key_config
{
	int gpio;
	unsigned int code;
	int active_flag;
};

int gpio_key_polled_init(void);

#endif
