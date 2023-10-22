#include <hal_gpio.h>

/* config the gpio to test */
#if defined(CONFIG_SOC_SUN8IW19P1) || defined(CONFIG_ARCH_SUN55IW3)
#define GPIO_TEST		GPIO_PC0
#else
#define GPIO_TEST		GPIO_PA1
#endif
