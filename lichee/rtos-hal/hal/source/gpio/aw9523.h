#ifndef __AW9523_I_H__
#define __AW9523_I_H__

#ifdef __cplusplus
extern "C" {
#endif

int aw9523_gpio_get_data(gpio_pin_t pin, gpio_data_t *data);
int aw9523_gpio_set_data(gpio_pin_t pin, gpio_data_t data);
int aw9523_gpio_set_direction(gpio_pin_t pin, gpio_direction_t direction);
int aw9523_gpio_get_direction(gpio_pin_t pin, gpio_direction_t *direction);
int aw9523_gpio_get_num(int port_num);
int aw9523_gpio_init(void);

#ifdef __cplusplus
}
#endif
#endif // __aw9523_I_H__
