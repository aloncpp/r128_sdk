#ifndef __PWM_LEDS_H
#define __PWM_LEDS_H


int sunxi_pwm_led_init(void);
void sunxi_set_pwm_led_brightness(unsigned int brightness);

#endif
