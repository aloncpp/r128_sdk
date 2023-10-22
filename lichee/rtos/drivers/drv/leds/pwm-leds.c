#include <stdio.h>
#include <string.h>
#include <sunxi_hal_pwm.h>
#include "./pwm-leds.h"

//#define PWM_DEBUG
#ifdef PWM_DEBUG
#define pwm_debug(fmt, args...) printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define pwm_debug(fmt, args...)
#endif

#define pwm_err(fmt, args...) printf("%s()%d - "fmt, __func__, __LINE__, ##args)

#define PWM_PERIOD_NS		10000
#define LED_MAX_BRIGHTNESS	255

enum pwm_led_channel {
	PWM_LED_RED,
	PWM_LED_GREEN,
	PWM_LED_BLUE,
	PWM_LED_COLOR,
};

struct pwm_led_config {
	const char *name;
	int pwm_channel;
};

int pwm_led_channel[PWM_LED_COLOR] = {-1};

#if defined(CONFIG_ARCH_SUN20IW2)
struct pwm_led_config pwm_config[3] = {
	{"red", 5},
	{"green", 4},
	{"blue", 6},
};
#else
struct pwm_led_config pwm_config[3] = {
	{"red", 1},
	{"green", 0},
	{"blue", 2},
};
#endif

static struct pwm_led_config *sunxi_get_pwm_config(void)
{
	return pwm_config;
}

int sunxi_pwm_led_init(void)
{
	int i;
	struct pwm_led_config *config;

	config = sunxi_get_pwm_config();
	if (NULL == config) {
		pwm_err("pwm get config fail\n");
		return -1;
	}

	hal_pwm_init();

	for (i = 0; i < 3; i++) {
		if (!strcmp(config[i].name, "red"))
				pwm_led_channel[PWM_LED_RED] = config[i].pwm_channel;

		if (!strcmp(config[i].name, "green"))
				pwm_led_channel[PWM_LED_GREEN] = config[i].pwm_channel;

		if (!strcmp(config[i].name, "blue"))
				pwm_led_channel[PWM_LED_BLUE] = config[i].pwm_channel;
	}

	if (pwm_led_channel[PWM_LED_RED] < 0 || pwm_led_channel[PWM_LED_GREEN] < 0
			|| pwm_led_channel[PWM_LED_BLUE] < 0) {
		pwm_err("get pwm led channel fail\n");
		return -1;
	}

	return 0;

}

void sunxi_set_pwm_led_brightness(unsigned int brightness)
{
	unsigned int duty_ns;
	unsigned char red, blue, green;
	struct pwm_config config_pwm;

	if (brightness > 0xFFFFFF) {
		pwm_err("brightness over range\n");
		return;
	}
	//pwm0_test();
	red = (brightness & 0xFF0000) >> 16;
	duty_ns = (red * PWM_PERIOD_NS)/LED_MAX_BRIGHTNESS;
	config_pwm.duty_ns = duty_ns;
	config_pwm.period_ns = PWM_PERIOD_NS;
	config_pwm.polarity = PWM_POLARITY_NORMAL;
	pwm_debug("red pwm channel = %d, duty_ns = %d\n", pwm_led_channel[PWM_LED_RED], duty_ns);
	hal_pwm_control(pwm_led_channel[PWM_LED_RED], &config_pwm);

	green = (brightness & 0x00FF00) >> 8;
	duty_ns = (green * PWM_PERIOD_NS)/LED_MAX_BRIGHTNESS;
	config_pwm.duty_ns = duty_ns;
	config_pwm.period_ns = PWM_PERIOD_NS;
	config_pwm.polarity = PWM_POLARITY_NORMAL;
	pwm_debug("green pwm channel = %d, duty_ns = %d\n", pwm_led_channel[PWM_LED_GREEN], duty_ns);
	hal_pwm_control(pwm_led_channel[PWM_LED_GREEN], &config_pwm);

	blue = brightness & 0x0000FF;
	duty_ns = (blue * PWM_PERIOD_NS)/LED_MAX_BRIGHTNESS;
	config_pwm.duty_ns = duty_ns;
	config_pwm.period_ns = PWM_PERIOD_NS;
	config_pwm.polarity = PWM_POLARITY_NORMAL;
	pwm_debug("blue pwm channel = %d, duty_ns = %d\n", pwm_led_channel[PWM_LED_BLUE], duty_ns);
	hal_pwm_control(pwm_led_channel[PWM_LED_BLUE], &config_pwm);
}
