/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*how to include the head file*/
#include "../../include/drivers/hal_gpio.h"
#include <stdio.h>
#include <tinatest.h>

#define IRQ_TYPE_NONE	0x00000000
#define IRQ_TYPE_EDGE_RISING	0x00000001
#define IRQ_TYPE_EDGE_FALLING		0x00000002
#define IRQ_TYPE_EDGE_BOTH	(IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING)
#define IRQ_TYPE_LEVEL_HIGH	0x00000004
#define IRQ_TYPE_LEVEL_LOW	0x00000008

int gpio_test(int argc, char **argv)
{
	hal_gpio_pull_down_t pull_state;
	hal_gpio_direction_t gpio_direction;
	hal_gpio_data_t gpio_data;

	hal_gpio_set_pull_state(HAL_GPIO_134, HAL_GPIO_PULL_UP);
	hal_gpio_set_direction(HAL_GPIO_134, HAL_GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_output(HAL_GPIO_134,HAL_GPIO_DATA_HIGH);
	printf("set pull state: %d, dir : %d, data : %d\n",HAL_GPIO_PULL_UP, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_HIGH);

	hal_gpio_get_pull_state(HAL_GPIO_134, &pull_state);
	hal_gpio_get_direction(HAL_GPIO_134, &gpio_direction);
	hal_gpio_get_output(HAL_GPIO_134, &gpio_data);

	printf("get pull state : %d, dir : %d, data :0x%0x\n",pull_state,gpio_direction,gpio_data);

	hal_gpio_set_pull_state(HAL_GPIO_133, HAL_GPIO_PULL_DOWN);
	hal_gpio_set_direction(HAL_GPIO_133, HAL_GPIO_DIRECTION_INPUT);
	printf("set pull state: %d, dir : %d\n",HAL_GPIO_PULL_DOWN, HAL_GPIO_DIRECTION_INPUT);
	//hal_gpio_set_output(HAL_GPIO_134,HAL_GPIO_DATA_HIGH);

	hal_gpio_get_pull_state(HAL_GPIO_133, &pull_state);
	hal_gpio_get_direction(HAL_GPIO_133, &gpio_direction);
	hal_gpio_get_output(HAL_GPIO_133, &gpio_data);

	printf("get pull state : %d, dir : %d, data : 0x%0x\n",pull_state,gpio_direction,gpio_data);

	hal_gpio_set_pull_state(HAL_GPIO_128, HAL_GPIO_PULL_DOWN);
	hal_gpio_set_direction(HAL_GPIO_128, HAL_GPIO_DIRECTION_INPUT);
	printf("set pull state: %d, dir : %d\n",HAL_GPIO_PULL_DOWN, HAL_GPIO_DIRECTION_INPUT);

	hal_gpio_get_pull_state(HAL_GPIO_128, &pull_state);
	hal_gpio_get_direction(HAL_GPIO_128, &gpio_direction);
	hal_gpio_get_output(HAL_GPIO_128, &gpio_data);

	printf("get pull state : %d, dir : %d, data : 0x%0x\n",pull_state,gpio_direction,gpio_data);

	hal_gpio_set_pull_state(HAL_GPIO_131, HAL_GPIO_PULL_UP);
	hal_gpio_set_direction(HAL_GPIO_131, HAL_GPIO_DIRECTION_OUTPUT);
	hal_gpio_set_output(HAL_GPIO_131,HAL_GPIO_DATA_LOW);
	printf("set pull state: %d, dir : %d, data : %d\n",HAL_GPIO_PULL_UP, HAL_GPIO_DIRECTION_OUTPUT, HAL_GPIO_DATA_LOW);

	hal_gpio_get_pull_state(HAL_GPIO_131, &pull_state);
	hal_gpio_get_direction(HAL_GPIO_131, &gpio_direction);
	hal_gpio_get_output(HAL_GPIO_131, &gpio_data);

	printf("get pull state : %d, dir : %d, data : 0x%0x\n",pull_state,gpio_direction,gpio_data);

	return 0;
}

static void gpio_irq_test(int unused, void *data)
{
	printf("run interrupt handle\n");
}

int gpio_irq_test_start(int argc, char **argv)
{
	uint32_t irq;
	int ret = 0;

	ret = hal_gpio_to_irq(HAL_GPIO_131, &irq);
	if (ret < 0) {
		printf("gpio to irq error, error num: %d\n", ret);
		return ret;
	}

	ret = hal_gpio_irq_request(irq, gpio_irq_test, IRQ_TYPE_EDGE_RISING, NULL);
	if (ret < 0) {
		printf("request irq error, irq num:%u error num: %d\n", irq, ret);
		return ret;
	}

	ret = hal_gpio_irq_enable(irq);
	if (ret < 0) {
		printf("request irq error, error num: %d\n", ret);
		return ret;
	}

	ret = hal_gpio_irq_disable(irq);
	if (ret < 0) {
		printf("disable irq error, irq num:%u,error num: %d\n", irq,ret);
		return ret;
	}
	return 0;
}

int gpio_irq_test_stop(int argc, char **argv)
{
	uint32_t irq;
	int ret = 0;

	ret = hal_gpio_to_irq(HAL_GPIO_131, &irq);
	if (ret < 0) {
		printf("gpio to irq error, error num: %d\n", ret);
		return ret;
	}

	ret = hal_gpio_irq_disable(irq);
	if (ret < 0) {
		printf("disable irq error, irq num:%u,error num: %d\n", irq,ret);
		return ret;
	}

	ret = hal_gpio_irq_free(irq);
	if (ret < 0) {
		printf("free irq error, error num: %d\n", ret);
		return ret;
	}
	return 0;
}


tt_funclist_t tt_gpiotest_funclist[] = {
	{"gpio interface test", gpio_test},
	{"gpio irq test start", gpio_irq_test_start},
	{"gpio irq test stop", gpio_irq_test_stop},
};

testcase_init_with_funclist(tt_gpiotest_funclist,gpio_test,testcase funclist test);

