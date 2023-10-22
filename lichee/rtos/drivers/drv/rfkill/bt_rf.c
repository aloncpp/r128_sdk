/*
 * Filename:bt_rf.c
 * description: bluetooth rfkill manager.
 * Created: 2019.07.22
 * Author:laumy
 */
//#include <stdbool.h>
#include <hal_gpio.h>
#include <stdio.h>

void mdelay(uint32_t ms);

int bt_rf_probe(void)
{
#ifdef CONFIG_ARCH_SUN8IW18P1
	hal_gpio_set_pull(GPIO_PE4, GPIO_PULL_UP);
	hal_gpio_set_direction(GPIO_PE4, GPIO_DIRECTION_OUTPUT);
#elif defined CONFIG_ARCH_SUN20IW2P1
#endif
}

int bt_rf_remove(void)
{
	;
}


int bt_set_power(bool on_off)
{
	;
	// 1. host wake bt pull up.
	// 2. power enable.
}

int bt_set_rfkill_state(bool on_off)
{
#ifdef CONFIG_ARCH_SUN8IW18P1
	if(on_off == true) {
		hal_gpio_set_data(GPIO_PE4, GPIO_DATA_HIGH);
		printf("wlan power on\n");
	}else if(on_off == false) {
		printf("wlan power off\n");
		hal_gpio_set_data(GPIO_PE4, GPIO_DATA_LOW);
	}
#elif defined CONFIG_ARCH_SUN20IW2P1
#endif
}

int bt_rfkill_state_reset(void)
{
	bt_set_rfkill_state(false);
	mdelay(100);
	//delay;
	bt_set_rfkill_state(true);
}
