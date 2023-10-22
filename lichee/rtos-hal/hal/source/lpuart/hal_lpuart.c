/**
 * @file  hal_lpuart.c
 * @author  XRADIO IOT WLAN Team
 */

/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <hal_lpuart.h>
#include <hal_interrupt.h>
#include <hal_queue.h>
#include <hal_debug.h>
#include <hal_clk.h>
#include <hal_reset.h>
#include <hal_gpio.h>
#include <hal_uart.h>
#include "lpuart.h"
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_wakesrc.h>
#include <pm_wakecnt.h>
#endif

#define LPUART_ERR(fmt, ...)  printf("lpuart: "fmt, ##__VA_ARGS__)
#define LPUART_INFO_IRQ(fmt, ...) printf("[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
#define LPUART_INFO(fmt, ...) printf("[%s %d]"fmt, __func__, __LINE__, ##__VA_ARGS__)
//#define LPUART_TEST

_lpuart_config_t lpuart_defconfig =
{
	.baudrate    = LPUART_BAUDRATE_9600,
	.word_length = LPUART_WORD_LENGTH_8,
	.msb_bit     = LPUART_MSB_BIT_0,
	.parity      = LPUART_PARITY_NONE,
};

static lpuart_priv_t g_lpuart_priv[LPUART_MAX];

static const uint32_t g_lpuart_baudrate_map[] =
{
	300,
	600,
	1200,
	2400,
	4800,
	9600
};
static bool lpuart_port_is_valid(lpuart_port_t lpuart_port)
{
	return (lpuart_port < LPUART_MAX);
}
/*
static bool uart_port_is_valid(uart_port_t uart_port)
{
	return (uart_port < UART_MAX);
}
*/
static hal_mailbox_t lpuart_mailbox[LPUART_MAX];

static bool lpuart_config_is_valid(const _lpuart_config_t *config)
{
	return ((config->baudrate < LPUART_BAUDRATE_MAX) &&
			(config->word_length <= LPUART_RCR_DWID9) &&
			(config->msb_bit <= LPUART_MSB_BIT_1) &&
			(config->parity <= LPUART_PARITY_MARK));
}

unsigned long sunxi_lpuart_port[] =
{
	SUNXI_LPUART0_BASE, SUNXI_LPUART1_BASE
};

static int lpuart_clk_init(int bus, bool enable)
{
	hal_clk_status_t ret;
	hal_reset_type_t reset_type = HAL_SUNXI_AON_RESET;
	u32 reset_id;
	hal_clk_type_t clk_type = HAL_SUNXI_AON_CCU;
	hal_clk_id_t clk_id, wkclk_id;
	hal_clk_t clk;
	struct reset_control *reset;

	switch (bus)
	{
		case 0:
			clk_id = SUNXI_CLK_LPUART0;
			wkclk_id = SUNXI_CLK_LPUART0_WKUP;
			reset_id = SUNXI_RST_LPUART0;
			break;
		case 1:
			clk_id = SUNXI_CLK_LPUART1;
			wkclk_id = SUNXI_CLK_LPUART1_WKUP;
			reset_id = SUNXI_RST_LPUART1;
			break;
		default:
			LPUART_ERR("lpuart%d is invalid\n", bus);
			return -1;
	}
	if (enable)
	{
		reset = hal_reset_control_get(reset_type, reset_id);
		hal_reset_control_reset(reset);
		hal_reset_control_put(reset);

		clk = hal_clock_get(clk_type, clk_id);
		ret = hal_clock_enable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't enable clk!\n", bus);
			return -1;
		}
		clk = hal_clock_get(clk_type, wkclk_id);
		ret = hal_clock_enable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't enable wkclk!\n", bus);
			return -1;
		}

	}
	else
	{
		clk = hal_clock_get(clk_type, clk_id);
		ret = hal_clock_disable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't disable clk!\n", bus);
			return -1;
		}
		clk = hal_clock_get(clk_type, wkclk_id);
		ret = hal_clock_disable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't disable wkclk!\n", bus);
			return -1;
		}

	}

	return 0;
}

#ifdef CONFIG_PM_WAKESRC_LPUART
static int lpuart_clk_start(int bus, bool enable)
{
	hal_clk_status_t ret;
	hal_clk_type_t clk_type = HAL_SUNXI_AON_CCU;
	hal_clk_id_t clk_id, wkclk_id;
	hal_clk_t clk;

	switch (bus)
	{
		case 0:
			clk_id = SUNXI_CLK_LPUART0;
			wkclk_id = SUNXI_CLK_LPUART0_WKUP;
			break;
		case 1:
			clk_id = SUNXI_CLK_LPUART1;
			wkclk_id = SUNXI_CLK_LPUART1_WKUP;
			break;
		default:
			LPUART_ERR("lpuart%d is invalid\n", bus);
			return -1;
	}
	if (enable)
	{
		clk = hal_clock_get(clk_type, clk_id);
		ret = hal_clock_enable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't enable clk!\n", bus);
			return -1;
		}
		clk = hal_clock_get(clk_type, wkclk_id);
		ret = hal_clock_enable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't enable wkclk!\n", bus);
			return -1;
		}

	}
	else
	{
		clk = hal_clock_get(clk_type, clk_id);
		ret = hal_clock_disable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't disable clk!\n", bus);
			return -1;
		}
		clk = hal_clock_get(clk_type, wkclk_id);
		ret = hal_clock_disable(clk);
		if (ret)
		{
			LPUART_ERR("[lpuart%d] couldn't disable wkclk!\n", bus);
			return -1;
		}

	}

	return 0;
}
#endif

static void lpuart_pinctrl_init(lpuart_port_t lpuart_port)
{
	switch (lpuart_port)
	{
		case LPUART_0:
			hal_gpio_pinmux_set_function(LPUART0_RX, LPUART0_GPIO_FUNCTION);//RX
			break;
		case LPUART_1:
			hal_gpio_pinmux_set_function(LPUART1_RX, LPUART1_GPIO_FUNCTION);//RX
			break;
		default:
			LPUART_ERR("[lpuart%d] not support \n", lpuart_port);
			break;
	}
}

static void lpuart_pinctrl_uninit(lpuart_port_t lpuart_port)
{
	switch (lpuart_port)
	{
		case LPUART_0:
			hal_gpio_pinmux_set_function(LPUART0_RX, GPIO_MUXSEL_DISABLED);//RX
			break;
		case LPUART_1:
			hal_gpio_pinmux_set_function(LPUART1_RX, GPIO_MUXSEL_DISABLED);//RX
			break;
		default:
			LPUART_ERR("[lpuart%d] not support \n", lpuart_port);
			break;
	}
}

static void lpuart_set_format(lpuart_port_t lpuart_port, lpuart_word_length_t word_length,
		lpuart_msb_bit_t msb_bit, lpuart_parity_t parity)
{
	const unsigned long lpuart_base = sunxi_lpuart_port[lpuart_port];
	uint32_t value;

	value = hal_readl(lpuart_base + LPUART_RCR);

	/* set data width */
	value &= ~(LPUART_RCR_DWID_MASK);
	switch (word_length)
	{
		case LPUART_WORD_LENGTH_4:
			value |= LPUART_RCR_DWID4;
			break;
		case LPUART_WORD_LENGTH_5:
			value |= LPUART_RCR_DWID5;
			break;
		case LPUART_WORD_LENGTH_6:
			value |= LPUART_RCR_DWID6;
			break;
		case LPUART_WORD_LENGTH_7:
			value |= LPUART_RCR_DWID7;
			break;
		case LPUART_WORD_LENGTH_8:
			value |= LPUART_RCR_DWID8;
			break;

		case LPUART_WORD_LENGTH_9:
		default:
			value |= LPUART_RCR_DWID9;
			break;
	}

	/* set msb bit */
	switch (msb_bit)
	{
		case LPUART_MSB_BIT_0:
		default:
			value &= ~(LPUART_RCR_MSB);
			break;
		case LPUART_MSB_BIT_1:
			value |= LPUART_RCR_MSB;
			break;
	}

	/* set parity bit */
	value &= ~(LPUART_RCR_PARITY_MASK);
	switch (parity)
	{
		case LPUART_PARITY_EVEN:
			value |= EVEN;
			break;
		case LPUART_PARITY_ODD:
			value |= ODD;
			break;
		case LPUART_PARITY_SPACE:
			value |= SPACE;
			break;
		case LPUART_PARITY_MARK:
			value |= MARK;
			break;
		case LPUART_PARITY_NONE:
		default:
			break;
	}
	value |= EN_RX;

	hal_writel(value, lpuart_base + LPUART_RCR);
}

static void lpuart_set_baudrate(lpuart_port_t lpuart_port, lpuart_baudrate_t baudrate)
{
	const unsigned long lpuart_base = sunxi_lpuart_port[lpuart_port];
	uint32_t divisor, quotient, reaminder;
	int32_t value;
	int32_t actual_baudrate = g_lpuart_baudrate_map[baudrate];

	/* read real rcosc_clk rate */
	value = (readl(0x40050140) & 0x0FFFFF00) >> 8;
	value = value / 10 * 2;

	divisor = actual_baudrate / 100;
	quotient = value / divisor;
	reaminder = value - quotient * divisor;

	value = (divisor << 24) | (reaminder << 16) | (quotient);

	hal_writel(value, lpuart_base + LPUART_BCR);
}

static const uint32_t g_lpuart_irqn[] = {SUNXI_IRQ_LPUART0, SUNXI_IRQ_LPUART1};

static void lpuart_enable_irq(lpuart_port_t lpuart_port, uint32_t irq_type)
{
	const unsigned long lpuart_base = sunxi_lpuart_port[lpuart_port];
	uint32_t value;

	if (irq_type) {
		value = hal_readl(lpuart_base + LPUART_IER);
		value |= irq_type;
	} else {
		value = 0;
	}
	hal_writel(value, lpuart_base + LPUART_IER);

}

static void lpuart_handle_rx_data(lpuart_port_t lpuart_port)
{
	const unsigned long lpuart_base = sunxi_lpuart_port[lpuart_port];
	uint8_t ch = 0;

	ch = hal_readb(lpuart_base + LPUART_RDR);
	hal_mailbox_send((hal_mailbox_t)lpuart_mailbox[lpuart_port], ch);

#ifdef LPUART_TEST
	printf("%s:%d, ch:0x%x\n", __func__, __LINE__, ch);
	if (lpuart_port == 0)
		printf("lpuart 0 recv data test success\n");
	if (lpuart_port == 1)
		printf("lpuart 1 recv data test success\n");
#endif
}

static void lpuart_handle_rx_data_cmp(lpuart_port_t lpuart_port)
{
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];

	if (lpuart_priv->func) {
		lpuart_priv->func(NULL);
	}

#ifdef CONFIG_PM_WAKESRC_LPUART
	pm_wakecnt_inc(g_lpuart_irqn[lpuart_port]);
#endif
#ifdef LPUART_TEST
	if (lpuart_port == 0)
		printf("lpuart 0 cmp data test success\n");
	if (lpuart_port == 1)
		printf("lpuart 1 cmp data test success\n");
#endif
}

void lpuart_multiplex(lpuart_port_t lpuart_port, uart_port_t uart_port)
{
	int32_t value;
	unsigned long base;

#ifdef CONFIG_PM_WAKESRC_LPUART
	uart_multiplex_lpuart_nosuspend(uart_port);
#else
	uart_multiplex_lpuart(uart_port);
#endif
	if (lpuart_port == 0) {
		base  = SUNXI_CCMU_AON_BASE + LPUART0_AON;
		value = hal_readl(base);
	} else if (lpuart_port == 1) {
		base  = SUNXI_CCMU_AON_BASE + LPUART1_AON;
		value = hal_readl(base);
	} else {
		LPUART_ERR("invalid lpuart port num\n");
	}

	switch (uart_port)
	{
		case UART_0:
				value &= ~LPUART_MULT_16;
				value &= ~LPUART_MULT_17;
				break;
		case UART_1:
				value |= LPUART_MULT_16;
				value &= ~LPUART_MULT_17;
				break;
		case UART_2:
				value &= ~LPUART_MULT_16;
				value |= LPUART_MULT_17;
				break;
		default:
				LPUART_ERR("invalied uart port num\n");
				break;
	}
	hal_writel(value, base);
}

int32_t hal_lpuart_stop(lpuart_port_t lpuart_port)
{
	lpuart_clk_init(lpuart_port, false);
	lpuart_pinctrl_uninit(lpuart_port);

	return SUNXI_HAL_OK;
}

#ifdef CONFIG_COMPONENTS_PM
static int hal_lpuart_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	lpuart_port_t lpuart_port = (lpuart_port_t)dev->data;

#ifdef CONFIG_PM_WAKESRC_LPUART
	/* Device should not suspend when acting as a wakeup source.
	 * If necessary, dO some wakeup preparation instead.
	 */
	lpuart_clk_start(lpuart_port, true);
	return 0;
#else
	LPUART_INFO("lpuart %d suspend\n", lpuart_port);

	hal_lpuart_stop(lpuart_port);
	return 0;
#endif
}

static int hal_lpuart_resume(struct pm_device *dev, suspend_mode_t mode)
{

	lpuart_port_t lpuart_port = (lpuart_port_t)dev->data;

#ifdef CONFIG_PM_WAKESRC_LPUART
	lpuart_clk_start(lpuart_port, false);
	return 0;
#else

	_lpuart_config_t *lpuart_config = &lpuart_defconfig;
	char lpuart_name[12] = {0};

	if ((!lpuart_port_is_valid(lpuart_port)) ||
			(!lpuart_config_is_valid(lpuart_config)))
	{
		hal_log_err("error parameter\r\n");
		return -1;
	}

	/* clk reset and init */
	lpuart_clk_init(lpuart_port, true);

	/* request gpio */
	lpuart_pinctrl_init(lpuart_port);

	/* config lpuart attributes */
	lpuart_set_format(lpuart_port, lpuart_config->word_length,
			lpuart_config->msb_bit, lpuart_config->parity);

	lpuart_set_baudrate(lpuart_port, lpuart_config->baudrate);
	sprintf(lpuart_name, "lpuart%d", (int)lpuart_port);

	LPUART_INFO("lpuart%d resume\n", lpuart_port);

	return 0;
#endif
}

struct pm_devops pm_lpuart_ops = {
	.suspend = hal_lpuart_suspend,
	.resume = hal_lpuart_resume,
};
#endif

bool hal_lpuart_is_init(lpuart_port_t lpuart_port)
{
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	return lpuart_priv->init;
}

static hal_irqreturn_t lpuart_irq_handler(void *dev_id)
{
	lpuart_priv_t *lpuart_priv = dev_id;
	lpuart_port_t lpuart_port = lpuart_priv->lpuart_port;
	const unsigned long lpuart_base = sunxi_lpuart_port[lpuart_port];
	uint32_t rd, rdc;

	rd = hal_readl(lpuart_base + LPUART_ISR) & LPUART_RX_DATA_FLAG_BIT;
	rdc = hal_readl(lpuart_base + LPUART_ISR) & LPUART_RX_DATA_CMP_FLAG_BIT;

	if (rd == LPUART_IER_RD)
	{
		lpuart_handle_rx_data(lpuart_port);
	}
	if (rdc == LPUART_IER_RDC)
	{
		lpuart_handle_rx_data_cmp(lpuart_port);
	}

	hal_writel(LPUART_IER_RDC | LPUART_IER_RD, lpuart_base + LPUART_ICR);
	return 0;
}

int32_t hal_lpuart_init(lpuart_port_t lpuart_port)
{
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	uint32_t irqn = g_lpuart_irqn[lpuart_port];
	_lpuart_config_t *lpuart_config = &lpuart_defconfig;
	char lpuart_name[12] = {0};

	if ((!lpuart_port_is_valid(lpuart_port)) ||
			(!lpuart_config_is_valid(lpuart_config)))
	{
		return HAL_LPUART_STATUS_ERROR_PARAMETER;
	}

	if (hal_lpuart_is_init(lpuart_port)) {
		LPUART_ERR("lpuart already inited\n");
		return SUNXI_HAL_OK;
	}

	/* enable clk */
	lpuart_clk_init(lpuart_port, true);

	/* request gpio */
	lpuart_pinctrl_init(lpuart_port);

	/* config lpuart attributes */

	sprintf(lpuart_name, "lpuart%d", (int)lpuart_port);
	lpuart_priv->lpuart_port = lpuart_port;
	lpuart_priv->irqn = irqn;

	if (hal_request_irq(irqn, lpuart_irq_handler, lpuart_name, lpuart_priv) < 0)
	{
		LPUART_ERR("request irq error\n");
		return HAL_LPUART_STATUS_ERROR;
	}

	hal_enable_irq(irqn);

	lpuart_mailbox[lpuart_port] = hal_mailbox_create(lpuart_name, LPUART_FIFO_SIZE);
	if (lpuart_mailbox[lpuart_port] == NULL)
	{
		LPUART_ERR("create mailbox fail\n");
		return HAL_LPUART_STATUS_ERROR;
	}

#ifdef CONFIG_COMPONENTS_PM
#ifdef CONFIG_PM_WAKESRC_LPUART
	int ret;
	ret = pm_wakesrc_register(irqn, "lpuart", PM_WAKESRC_ALWAYS_WAKEUP);
	if (ret)
		LPUART_ERR("lpuart registers wakesrc fail\n");
	ret = pm_set_wakeirq(irqn);
	if (ret)
		LPUART_ERR("pm_set_wakeirq fail\n");
#endif

	lpuart_priv->pm.data = (void *)lpuart_port;
	lpuart_priv->pm.name = "sunxi_lpuart";
	lpuart_priv->pm.ops = &pm_lpuart_ops;
	pm_devops_register(&lpuart_priv->pm);
#endif

	lpuart_priv->init = true;
	return SUNXI_HAL_OK;
}


int32_t hal_lpuart_deinit(lpuart_port_t lpuart_port)
{
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	uint32_t irqn = g_lpuart_irqn[lpuart_port];

#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&lpuart_priv->pm);
#ifdef CONFIG_PM_WAKESRC_LPUART
	int ret;
	ret = pm_clear_wakeirq(irqn);
	if (ret)
		LPUART_ERR("pm_clear_wakeirq fail\n");
	ret = pm_wakesrc_unregister(irqn);
	if (ret)
		LPUART_ERR("lpuart unregisters wakesrc fail\n");
#endif
#endif

	lpuart_clk_init(lpuart_port, false);
	lpuart_pinctrl_uninit(lpuart_port);
	lpuart_enable_irq(lpuart_port, 0);
	hal_disable_irq(irqn);
	hal_free_irq(irqn);

	lpuart_priv->init = false;
	return SUNXI_HAL_OK;
}

int32_t hal_lpuart_control(lpuart_port_t lpuart_port, void *args)
{
	_lpuart_config_t *lpuart_config;
	lpuart_config = (_lpuart_config_t *)args;

	if (lpuart_config->baudrate >= LPUART_BAUDRATE_MAX) {
		LPUART_ERR("unsupport baudrate, use default 9600\n");
		lpuart_config->baudrate =  LPUART_BAUDRATE_9600;
	}

	/* config lpuart attributes */
	lpuart_set_format(lpuart_port, lpuart_config->word_length,
			lpuart_config->msb_bit, lpuart_config->parity);
	lpuart_set_baudrate(lpuart_port, lpuart_config->baudrate);

	return SUNXI_HAL_OK;
}

int32_t hal_lpuart_receive(int32_t dev, uint8_t *data, uint32_t num)
{
	unsigned int data_rev;
	int i = 0;
	int32_t ret = -1, rev_count = 0;

	hal_assert(data != NULL);

	for (i = 0; i < num; i++)
	{
		ret = hal_mailbox_recv((hal_mailbox_t)lpuart_mailbox[dev], &data_rev, -1);
		if (ret == 0)
		{
			rev_count++;
			*(data + i) = (uint8_t)data_rev;
		}
		else
		{
			LPUART_ERR("receive error");
			break;
		}
	}

	return rev_count;
}

/**
 * @brief setup rx data compare function
 * @param[in] lpuart_port ID of the specified LPUART
 * @param[in] cmp_len the length of compare data
 * @param[in] cmp_data buffer point to compare data
 * @retval HAL_Status, SUNXI_HAL_OK on success
 *
 * @note
 */

int32_t hal_lpuart_rx_cmp(lpuart_port_t lpuart_port, uint8_t cmp_len, uint8_t *cmp_data)
{
	uint32_t i, reg_value1 = 0, reg_value2 = 0;
	uint8_t pos[5] = {LPUART_RX_CMP_DATA0_POS, LPUART_RX_CMP_DATA1_POS,
		LPUART_RX_CMP_DATA2_POS, LPUART_RX_CMP_DATA3_POS, LPUART_RX_CMP_DATA4_POS};

	if ((cmp_data == NULL) || (cmp_len == 0) || (cmp_len > LPUART_RX_CMP_DATA_NUM_MAX)) {
		return HAL_LPUART_STATUS_ERROR;
	}

	for (i = 0; i < cmp_len; i++) {
		if (i >= 3) {
			reg_value2 |= cmp_data[i] << pos[i];
		} else {
			reg_value1 |= cmp_data[i] << pos[i];
		}
	}

	if (!hal_lpuart_is_init(lpuart_port)) {
		LPUART_ERR("lpuart %d not inited\n", lpuart_port);
		return HAL_LPUART_STATUS_ERROR;
	}

	const unsigned long lpuart_base = sunxi_lpuart_port[lpuart_port];
	hal_writel(reg_value1 | (cmp_len << LPUART_RX_CMP_LEN_SHIFT), lpuart_base + LPUART_RCP1);
	hal_writel(reg_value2, lpuart_base + LPUART_RCP2);

	lpuart_enable_irq(lpuart_port, LPUART_IER_RDC);
	return SUNXI_HAL_OK;
}

int32_t hal_lpuart_enable_rx_data(lpuart_port_t lpuart_port, lpuart_callback_t cb, void *arg)
{
	if (!hal_lpuart_is_init(lpuart_port)) {
		LPUART_ERR("lpuart %d not inited\n", lpuart_port);
		return HAL_LPUART_STATUS_ERROR;
	}
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	lpuart_priv->func = cb;
	lpuart_priv->arg = arg;
	lpuart_enable_irq(lpuart_port, LPUART_IER_RD);
	return SUNXI_HAL_OK;
}

int32_t hal_lpuart_disable_rx_data(lpuart_port_t lpuart_port)
{
	if (!hal_lpuart_is_init(lpuart_port)) {
		LPUART_ERR("lpuart %d not inited\n", lpuart_port);
		return HAL_LPUART_STATUS_ERROR;
	}
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	lpuart_enable_irq(lpuart_port, 0);
	lpuart_priv->func = NULL;
	return SUNXI_HAL_OK;
}

int32_t hal_lpuart_enable_rx_cmp(lpuart_port_t lpuart_port, lpuart_callback_t cb, void *arg)
{
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	lpuart_priv->func = cb;
	lpuart_priv->arg = arg;
	lpuart_enable_irq(lpuart_port, LPUART_IER_RDC);
	return SUNXI_HAL_OK;
}

/**
 * @brief Disable receive compare callback function for the specified LPUART
 * @param[in] lpuart_port ID of the specified LPUART
 * @retval HAL_Status, SUNXI_HAL_OK on success
 */
int32_t hal_lpuart_disable_rx_cmp(lpuart_port_t lpuart_port)
{
	if (!hal_lpuart_is_init(lpuart_port)) {
		LPUART_ERR("lpuart %d not inited\n", lpuart_port);
		return HAL_LPUART_STATUS_ERROR;
	}
	lpuart_priv_t *lpuart_priv = &g_lpuart_priv[lpuart_port];
	lpuart_enable_irq(lpuart_port, 0);
	lpuart_priv->func = NULL;
	return SUNXI_HAL_OK;
}
