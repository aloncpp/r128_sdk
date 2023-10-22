#include "gpio.h"

#if defined(CONFIG_OS_TINA)
#include <hal_interrupt.h>
#endif

#ifdef __MELIS__
__hdle			 xr_hPin;

int gpio_direction_output(unsigned gpio, int value)
{
	return 0;
}

void xr_wlan_irq_subscribe(void *func)
{
    user_gpio_set_t  PinStat;
    //__bool           IntStat;

    //request C500-SOC PD13(EINTD13) as int pin,
    //initialize pin status information
    eLIBs_strcpy(PinStat.gpio_name, "EINTD13");
    PinStat.port      = 4;
    PinStat.port_num  = 13;
    PinStat.mul_sel   = 6;  //EINTD13
    PinStat.pull      = 1;
    PinStat.drv_level = 1;
    PinStat.data      = 1;

    //request int pin
    xr_hPin = esPINS_PinGrpReq(&PinStat, 1);
    if (xr_hPin == NULL) {
        __wrn("Request DINT13 pin failed\n");
        return ;
    }
    __inf("Request DINT13 pin succeeded\n");

    //register pin int handler
    if (esPINS_RegIntHdler(xr_hPin, (__pCBK_t)func, NULL) != EPDK_OK) {
        __wrn("register pin int handler failed\n");
    }

    __inf("esPINS_RegIntHdler succeeded\n");
    //set pin int mode
    if (esPINS_SetIntMode(xr_hPin, PIN_INT_POSITIVE_EDGE) != EPDK_OK) {
        __wrn("set int mode failed\n");
    }

    __inf("esPINS_SetIntMode succeeded\n");
    //enable pin int
    esPINS_EnbaleInt(xr_hPin);

    __inf("%s succeeded\n", __func__);
    return ;
}

void xr_wlan_irq_unsubscribe(void *func)
{
	 //disable pin int
	 esPINS_DisbaleInt(xr_hPin);

	 //un-register pin int handler
	 esPINS_UnregIntHdler(xr_hPin, (__pCBK_t)func);

	 __inf("%s succeeded\n", __func__);
	 return;
}

#elif (defined(CONFIG_ARCH_STM32))

void xr_wlan_interrupt_init(void);
void xr_wlan_interrupt_exit(void);

void xr_wlan_irq_subscribe(void *func)
{
	xr_wlan_interrupt_init();
}

void xr_wlan_irq_unsubscribe(void *func)
{
	xr_wlan_interrupt_exit();
}

#elif defined(CONFIG_CHIP_XR876)

#include "driver/chip/hal_gpio.h"

void xr_wlan_irq_subscribe(void *func)
{
	GPIO_InitParam param;

	param.driving = GPIO_DRIVING_LEVEL_2;
	param.mode = GPIOx_Pn_F6_EINT;
	param.pull = GPIO_PULL_UP;
	HAL_GPIO_Init(GPIO_PORT_A, GPIO_PIN_17, &param);

	HAL_GPIO_WritePin(GPIO_PORT_A, GPIO_PIN_17, 1);
	GPIO_IrqParam Irq_param;

	Irq_param.event = GPIO_IRQ_EVT_RISING_EDGE;
	Irq_param.callback = func;
	Irq_param.arg = NULL;
	HAL_GPIO_EnableIRQ(GPIO_PORT_A, GPIO_PIN_17, &Irq_param);
}

__nonxip_text
void xr_wlan_irq_clean(void)
{
}

__nonxip_text
void xr_wlan_irq_enable(void)
{
}

__nonxip_text
void xr_wlan_irq_enable_clr(void)  /* no use */
{
}

__nonxip_text
void xr_wlan_irq_disable(void)
{
}

void xr_wlan_irq_unsubscribe(void *func)
{
}

#elif (defined(CONFIG_OS_RTTHREAD))

#ifndef CONFIG_USE_RTTHREAD_PORT
#include "board.h"
#include "drivers/pin.h"

struct rt_device xr_wlan_gpio;

#define PE5_INDEX 44 /* 44 is E5 */

void xr_wlan_irq_subscribe(void *func)
{
	//gpio_set_func(GPIO_PORT_E, GPIO_PIN_5, IO_INPUT(0));
	//gpio_set_pull_mode(GPIO_PORT_E, GPIO_PIN_5, PULL_UP);
	rt_pin_mode(PE5_INDEX, PIN_MODE_INPUT_PULLUP);
	gpio_set_drive_level(GPIO_PORT_E, GPIO_PIN_5, DRV_LEVEL_2); /* no rt pin interface, use aw's. */

	rt_pin_attach_irq(PE5_INDEX, PIN_IRQ_MODE_RISING, func, (void *)NULL);
	rt_pin_irq_enable(PE5_INDEX, PIN_IRQ_ENABLE);
}

__nonxip_text
void xr_wlan_irq_enable(void)
{
	rt_pin_irq_enable(PE5_INDEX, PIN_IRQ_ENABLE);
}

__nonxip_text
void xr_wlan_irq_disable(void)
{
	rt_pin_irq_enable(PE5_INDEX, PIN_IRQ_DISABLE);
}

void xr_wlan_irq_unsubscribe(void *func)
{
	rt_pin_detach_irq(PE5_INDEX);
}

#endif

#elif (defined(CONFIG_OS_NUTTX))
#include "config.h"
void xr_wlan_irq_subscribe(void *func)
{
	uint32_t irq_num;

	hal_gpio_set_pull(WL_WAKE_AP_PIN, GPIO_PULL_UP);
	hal_gpio_set_direction(WL_WAKE_AP_PIN, GPIO_DIRECTION_INPUT);	/*pin_x mode */
	hal_gpio_set_driving_level(WL_WAKE_AP_PIN, GPIO_DRIVING_LEVEL2);
	hal_gpio_to_irq(WL_WAKE_AP_PIN, &irq_num);
	hal_gpio_irq_request(irq_num, func, IRQ_TYPE_EDGE_RISING, (void *)NULL);
	hal_gpio_irq_enable(irq_num);

}

//__nonxip_text
void xr_wlan_irq_enable(void)
{
	uint32_t irq_num;

	hal_gpio_to_irq(WL_WAKE_AP_PIN, &irq_num);
	hal_gpio_irq_enable(irq_num);
}

void xr_wlan_irq_enable_clr(void)  /* no use */
{
}

//__nonxip_text
void xr_wlan_irq_disable(void)
{
	uint32_t irq_num;

	hal_gpio_to_irq(WL_WAKE_AP_PIN, &irq_num);
	hal_gpio_irq_disable(irq_num);
}

void xr_wlan_irq_unsubscribe(void *func)
{
	uint32_t irq_num;

	hal_gpio_to_irq(WL_WAKE_AP_PIN, &irq_num);
	hal_gpio_irq_free(irq_num);
}

static void wlan_power_gpio_init(int state)
{
#ifdef WL_SDC_EN_PIN
	hal_gpio_set_pull(WL_SDC_EN_PIN, GPIO_PULL_UP);
	hal_gpio_set_direction(WL_SDC_EN_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_driving_level(WL_SDC_EN_PIN, GPIO_DRIVING_LEVEL2);
#endif
	if (state)
		hal_gpio_set_pull(WL_REG_ON_PIN, GPIO_PULL_UP);
	else
		hal_gpio_set_pull(WL_REG_ON_PIN, GPIO_PULL_DOWN);

	hal_gpio_set_direction(WL_REG_ON_PIN, GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_driving_level(WL_REG_ON_PIN, GPIO_DRIVING_LEVEL2);
}

int xradio_wlan_gpio_power(int on)
{
	wlan_power_gpio_init(on);

	if (on) {
#ifdef WL_SDC_EN_PIN
		hal_gpio_set_data(WL_SDC_EN_PIN, GPIO_DATA_HIGH);
		XR_OS_MSleep(200);
#endif
		hal_gpio_set_data(WL_REG_ON_PIN, GPIO_DATA_HIGH);
		XR_OS_MSleep(10);
		hal_gpio_set_data(WL_REG_ON_PIN, GPIO_DATA_LOW);
		XR_OS_MSleep(20);
		hal_gpio_set_data(WL_REG_ON_PIN, GPIO_DATA_HIGH);
		XR_OS_MSleep(500);
	} else {
		hal_gpio_set_data(WL_REG_ON_PIN, GPIO_DATA_LOW);
		XR_OS_MSleep(50);
#ifdef WL_SDC_EN_PIN
		hal_gpio_set_data(WL_SDC_EN_PIN, GPIO_DATA_LOW);
#endif
	}

	return 0;
}
#elif defined(CONFIG_OS_TINA)
void xr_wlan_irq_subscribe(void *func)
{
	hal_request_irq(WLAN_IRQn, func, "wlan", (void *)NULL);
	hal_nvic_irq_set_priority(WLAN_IRQn, NVIC_WLAN_PRIO);
	hal_interrupt_clear_pending(WLAN_IRQn);
	hal_enable_irq(WLAN_IRQn);
}

__nonxip_text
void xr_wlan_irq_enable(void)
{
	hal_enable_irq(WLAN_IRQn);
}

__nonxip_text
void xr_wlan_irq_enable_clr(void)  /* no use */
{
	//nvic_clear_pending(WLAN_IRQn);
	hal_enable_irq(WLAN_IRQn);
}

__nonxip_text
void xr_wlan_irq_disable(void)
{
	hal_disable_irq(WLAN_IRQn);
}

void xr_wlan_irq_unsubscribe(void *func)
{
	hal_disable_irq(WLAN_IRQn);
	hal_free_irq(WLAN_IRQn);
}

#endif /* __MELIS__ */
