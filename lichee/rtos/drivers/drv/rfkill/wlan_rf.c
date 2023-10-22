/*
 * Filename:wlan_rf.c
 * description: wlan rfkill manager.
 * Created: 2019.07.22
 * Author:laumy
 */
//#include <stdbool.h>
#include <hal_gpio.h>
#include <stdio.h>

#ifdef CONFIG_ARCH_SUN20IW2P1
extern int xradio_wlan_power(int on);
#endif

int wlan_rf_probe(void)
{
	;
}

int wlan_rf_remove(void)
{
	;
}


int wlan_get_bus_index(void)
{
	int index = 1;

	return index;
}

int wlan_set_power(bool on_off)
{
#ifdef CONFIG_ARCH_SUN8IW18P1
	if(on_off == true) {
		printf("wlan power on\n");
		hal_gpio_set_pull(GPIO_PE6, GPIO_PULL_UP);
		hal_gpio_set_direction(GPIO_PE6, GPIO_DIRECTION_OUTPUT);
		hal_gpio_set_data(GPIO_PE6, GPIO_DATA_HIGH);
	}else if(on_off == false) {
		printf("wlan power off\n");
		hal_gpio_set_direction(GPIO_PE6, GPIO_DIRECTION_OUTPUT);
		hal_gpio_set_data(GPIO_PE6, GPIO_DATA_HIGH);
	}
#elif defined CONFIG_ARCH_SUN20IW2P1
	xradio_wlan_power(on_off);
#endif
}
