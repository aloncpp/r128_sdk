/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY��S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS��SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY��S TECHNOLOGY.
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

#include <stdint.h>

#include <hal_log.h>
#include <hal_cmd.h>
#include <hal_interrupt.h>
#include <hal_gpio.h>

#include "test_conf.h"

#define GPIO_PORT_MAX (8)

#ifdef CONFIG_ARCH_SUN20IW3
static int pins_number[GPIO_PORT_MAX] = {
    22, /* PA pins num */
    12, /* PC pins num */
    23, /* PD pins num */
    18, /* PE pins num */
    7,  /* PF pins num */
    8,  /* PG pins num */
    16, /* PH pins num */
    5,  /* PI pins num */
};
#else
static int pins_number[GPIO_PORT_MAX] = {
    0,
};
#endif

static void cmd_usage(void)
{
	printf("Usage:\n"
		"\t hal_gpio_cmd <cmd> <gpio> <arg>\n");
}

enum {
	GPIO_CMD_SET_VOL = 0,
};

static hal_irqreturn_t gpio_irq_test(void *data)
{
    hal_log_info("fake gpio interrupt handler");

    return 0;
}

int cmd_test_gpio(int argc, char **argv)
{
    uint32_t irq;
    int ret = 0;
    gpio_pull_status_t pull_state;
    gpio_direction_t gpio_direction;
    gpio_data_t gpio_data;
    gpio_muxsel_t function_index;

    hal_gpio_get_pull(GPIO_TEST, &pull_state);
    hal_gpio_get_direction(GPIO_TEST, &gpio_direction);
    hal_gpio_get_data(GPIO_TEST, &gpio_data);
    hal_gpio_pinmux_get_function(GPIO_TEST,&function_index);

    hal_log_info("Original: pin: %d pull state: %d, dir: %d, data: 0x%0x, function_index: %d",
                 GPIO_TEST, pull_state, gpio_direction, gpio_data, function_index);

    hal_log_info("Setting: pin: %d pull state: %d, dir: %d, data: 0x%x, function_index: %d",
                 GPIO_TEST, GPIO_PULL_UP, GPIO_DIRECTION_OUTPUT, GPIO_DATA_HIGH, GPIO_MUXSEL_OUT);

    hal_gpio_set_pull(GPIO_TEST, GPIO_PULL_UP);
    hal_gpio_set_direction(GPIO_TEST, GPIO_DIRECTION_OUTPUT);
    hal_gpio_set_data(GPIO_TEST, GPIO_DATA_HIGH);
    hal_gpio_pinmux_set_function(GPIO_TEST,GPIO_MUXSEL_OUT);

    hal_gpio_get_pull(GPIO_TEST, &pull_state);
    hal_gpio_get_direction(GPIO_TEST, &gpio_direction);
    hal_gpio_get_data(GPIO_TEST, &gpio_data);
    hal_gpio_pinmux_get_function(GPIO_TEST,&function_index);

    hal_log_info("Results: pin: %d pull state: %d, dir: %d, data: 0x%0x, function_index: %d",
                 GPIO_TEST, pull_state, gpio_direction, gpio_data, function_index);

    if (pull_state == GPIO_PULL_UP
            && gpio_direction == GPIO_DIRECTION_OUTPUT
            && gpio_data == GPIO_DATA_HIGH
	    && function_index == GPIO_MUXSEL_OUT)
    {
        hal_log_info("Test hal_gpio_set_pull API success!");
        hal_log_info("Test hal_gpio_set_direction API success!");
        hal_log_info("Test hal_gpio_set_data API success!");
        hal_log_info("Test hal_gpio_pinmux_set_function API success!");
        hal_log_info("Test hal_gpio_get_pull API success!");
        hal_log_info("Test hal_gpio_get_direction API success!");
        hal_log_info("Test hal_gpio_get_data API success!");
        hal_log_info("Test hal_gpio_pinmux_get_function API success!");
    } else {
        hal_log_err("Test API fail");
        goto failed;
    }

    ret = hal_gpio_to_irq(GPIO_TEST, &irq);
    if (ret < 0)
    {
        hal_log_err("gpio to irq error, irq num:%d error num: %d", irq, ret);
        goto failed;
    } else {
        hal_log_info("Test hal_gpio_to_irq API success!");
    }

    ret = hal_gpio_irq_request(irq, gpio_irq_test, IRQ_TYPE_EDGE_RISING, NULL);
    if (ret < 0)
    {
        hal_log_err("request irq error, irq num:%d error num: %d", irq, ret);
        goto failed;
    } else {
        hal_log_info("Test hal_gpio_irq_request API success!");
    }

    ret = hal_gpio_irq_enable(irq);
    if (ret < 0)
    {
        hal_log_err("request irq error, error num: %d", ret);
        goto failed;
    } else {
        hal_log_info("Test hal_gpio_irq_enable API success!");
    }

    ret = hal_gpio_irq_disable(irq);
    if (ret < 0)
    {
        hal_log_err("disable irq error, irq num:%d, error num: %d", irq, ret);
        goto failed;
    } else {
        hal_log_info("Test hal_gpio_irq_disable API success!");
    }

    ret = hal_gpio_irq_free(irq);
    if (ret < 0)
    {
        hal_log_err("free irq error, error num: %d", ret);
        goto failed;
    } else {
        hal_log_info("Test hal_gpio_irq_free API success!");
    }

    hal_log_info("Test gpio hal APIs success!");

    return 0;

failed:
    hal_log_err("Test gpio hal APIs failed!");
    return -1;
}

int cmd_test_gpio_all(int argc, char **argv)
{
    int i = 0;
    int j =0;
    int cnt = 0;
    int ret = 0;
    uint32_t irq;
    gpio_pin_t pin;
    gpio_pull_status_t pull_state;
    gpio_direction_t gpio_direction;
    gpio_data_t gpio_data;

    hal_log_info("The program will test all gpio hal APIs ...\n");

    for(i = 0; i < GPIO_PORT_MAX; i++)
    {
        for(j = 0; j < pins_number[i]; j++)
        {
            switch(i)
            {
            case 0: pin = GPIOA(j); break;
            case 1: pin = GPIOC(j); break;
            case 2: pin = GPIOD(j); break;
            case 3: pin = GPIOE(j); break;
            case 4: pin = GPIOF(j); break;
            case 5: pin = GPIOG(j); break;
            case 6: pin = GPIOH(j); break;
            case 7: pin = GPIOI(j); break;
            default: break;
            }
            hal_log_info("Setting: pull state: %d, dir: %d, data: 0x%x, pin: %d",
                            GPIO_PULL_DOWN, GPIO_DIRECTION_INPUT, GPIO_DATA_LOW, pin);

            hal_gpio_set_pull(pin, GPIO_PULL_DOWN);
            hal_gpio_set_direction(pin, GPIO_DIRECTION_INPUT);
            hal_gpio_set_data(pin, GPIO_DATA_LOW);

            hal_gpio_get_pull(pin, &pull_state);
            hal_gpio_get_direction(pin, &gpio_direction);
            hal_gpio_get_data(pin, &gpio_data);

            hal_log_info("Results: pull state: %d, dir: %d, data: 0x%0x",
                            pull_state, gpio_direction, gpio_data);

            if(pull_state != GPIO_PULL_DOWN
                   || gpio_direction != GPIO_DIRECTION_INPUT
                   || gpio_data != GPIO_DATA_LOW)
                    goto failed;

            ret = hal_gpio_to_irq(pin, &irq);
            if(ret < 0)
                    goto failed;

            ret = hal_gpio_irq_request(irq, gpio_irq_test, IRQ_TYPE_EDGE_FALLING, NULL);
            if(ret < 0)
                    goto failed;

            ret = hal_gpio_irq_enable(irq);
            if(ret < 0)
                    goto failed;

            ret = hal_gpio_irq_disable(irq);
            if(ret < 0)
                    goto failed;

            ret = hal_gpio_irq_free(irq);
            if(ret < 0)
                    goto failed;

            cnt++;
            hal_log_info("Test-%d: gpio pin %d hal success!\n", cnt, pin);
        }
    }

    hal_log_info("Test all gpio hal APIs success, cnt: %d!", cnt);
    return 0;

failed:
    hal_log_err("Test all gpio hal APIs failed!");
    return -1;
}

int cmd_test_gpio_cmd(int argc, char **argv)
{
    int cmd, gpio, arg;

    if (argc != 4)
        cmd_usage();

    cmd = strtol(argv[1], NULL, 0);
    gpio = strtol(argv[2], NULL, 0);
    arg = strtol(argv[3], NULL, 0);

    switch (cmd) {
        case GPIO_CMD_SET_VOL:
            hal_gpio_sel_vol_mode(gpio, arg);
            break;
        default:
            break;
    }

    return 0;
}
#ifdef CONFIG_DRIVERS_GPIO_EX_AW9523
int cmd_test_gpio_aw95xx(int argc, char **argv)
{
    int index;
    gpio_data_t data;
    gpio_direction_t dir;
    for(int i = 0; i < argc; i++)
        printf("argv[%d] = %s\n", i, argv[i]);

    if(argc == 2) {
        index = atoi(argv[1]);
        printf("index = %d\n", index);
        hal_gpio_get_data(GPIOEXP0(index), &data);
        printf("get aw95xx gpio %d = %d\n", index, data);
    } else if(argc == 3) {
        if(!strcmp("dir", argv[1])) {
            index = atoi(argv[2]);
            hal_gpio_get_direction(GPIOEXP0(index), &dir);
            printf("get aw95xx gpio %d dir = %d\n", index, dir);
        } else {
            index = atoi(argv[1]);
            data = atoi(argv[2]);
            hal_gpio_set_data(GPIOEXP0(index), data);
            printf("set aw95xx gpio %d = %d\n", index, data);
        }
    }  else if(argc == 4) {
        index = atoi(argv[2]);
        dir = atoi(argv[3]);
        hal_gpio_set_direction(GPIOEXP0(index), dir);
        printf("set aw95xx gpio %d dir = %d\n", index, dir);
    } else {
        printf("%s 0: get gpio 0\n", argv[0]);
        printf("%s 1 0: set gpio 1 to 0\n", argv[0]);
        printf("%s dir 0: get gpio 0 dir\n", argv[0]);
        printf("%s dir 2 1: set gpio 2 dir to 1\n", argv[0]);
    }
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_test_gpio_aw95xx, aw95xx_test, aw gpio hal APIs tests);
#endif

FINSH_FUNCTION_EXPORT_ALIAS(cmd_test_gpio, hal_gpio, gpio hal APIs tests);
FINSH_FUNCTION_EXPORT_ALIAS(cmd_test_gpio_cmd, hal_gpio_cmd, gpio hal APIs tests with cmd);
FINSH_FUNCTION_EXPORT_ALIAS(cmd_test_gpio_all, hal_gpio_all, gpio hal all APIs tests);
