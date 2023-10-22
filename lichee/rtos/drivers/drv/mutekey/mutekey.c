#include <drivers/hal_rtc.h>
#include <sunxi_hal_watchdog.h>
#ifdef CONFIG_DRIVERS_MUTEKEY_GPIO
#include <hal_gpio.h>
#endif


#ifdef CONFIG_DRIVERS_MUTEKEY_GPIO

#define MUTEKEY_GPIO HAL_GPIO_44 //GPB12

static void __mute_key_jump_fel_gpio(void)
{
	int loop = 5;
	hal_gpio_data_t gpio_data = 1;

	hal_gpio_set_direction(MUTEKEY_GPIO, HAL_GPIO_DIRECTION_INPUT);

	for (; loop > 0; loop--) {
		hal_gpio_get_input(MUTEKEY_GPIO, &gpio_data);
		if (gpio_data == 0) {
			printf("get mute key input,now jump to fel\n\n");
			hal_rtc_set_fel_flag();
			hal_spinor_deinit();
			hal_watchdog_restart();
		}
	}
}

#endif

int mute_key_jump_fel(void)
{
#ifdef CONFIG_DRIVERS_MUTEKEY_GPIO
	__mute_key_jump_fel_gpio();
#endif
	return 0;
}
