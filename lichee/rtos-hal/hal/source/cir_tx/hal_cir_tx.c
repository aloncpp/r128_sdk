/* CIR_TX
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <interrupt.h>
#include <hal_clk.h>
#include <hal_gpio.h>
#include <hal_reset.h>
#include <hal_mem.h>

#include "platform_cir_tx.h"
#include "sunxi_hal_cir_tx.h"

//#define CIR_TX_DEBUG /* define this macro when debugging is required */
#ifdef CIR_TX_DEBUG
#define CIR_TX_DBG(fmt, args...)  printf("%s()%d - "fmt, __func__, __LINE__, ##args)
#else
#define CIR_TX_DBG(fmt, args...)
#endif

#define CIR_TX_ERR(fmt, arg...) printf("%s()%d " fmt, __func__, __LINE__, ##arg)
#define CIR_MASTER_NUM	2


static uint32_t reg_base[CIR_MASTER_NUM] = {
	SUNXI_IRTX_PBASE,
};

static uint32_t irq[CIR_MASTER_NUM] = {
	SUNXI_IRQ_IRTX,
};

static cir_gpio_t pin[CIR_MASTER_NUM] = {
	{IRADC_PIN, IR_MUXSEL, 0},
};

static unsigned int three_pulse_cycle = 32;

static struct cir_tx_raw_buffer cir_raw_buffer;

static int cir_tx_get_intsta(void)
{
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	return readl(reg_addr + IR_TX_STAR);
}

static int cir_tx_fifo_empty(void)
{
	unsigned long reg_addr = SUNXI_IRTX_PBASE + IR_TX_TACR;
	unsigned int reg_val;

	reg_val = readl(reg_addr);

	return (reg_val == IR_TX_FIFO_SIZE);
}

static int cir_tx_fifo_full(struct sunxi_cir_tx_t *cir_tx)
{
	unsigned int reg_val;
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	reg_val = readl(reg_addr + IR_TX_TACR);

	return (reg_val == 0);
}

void cir_tx_reset_rawbuffer(void)
{
	int i;

	cir_raw_buffer.tx_dcnt = 0;
	for (i = 0; i < IR_TX_RAW_BUF_SIZE; i++)
		cir_raw_buffer.tx_buf[i] = 0;
}

static int cir_tx_clr_intsta(unsigned int bitmap)
{
	unsigned int reg_val;
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	reg_val = readl(reg_addr + IR_TX_STAR);
	reg_val &= ~0xff;
	reg_val |= bitmap & 0xff;
	writel(reg_val, reg_addr + IR_TX_STAR);
}

static int hal_cir_reg_clear(void)
{
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	writel(0, reg_addr + IR_TX_GLR);
}

void hal_cir_reg_cfg(void)
{
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	writel(IR_TX_MC_VALUE, reg_addr + IR_TX_MCR);
	writel(IR_TX_CLK_VALUE, reg_addr + IR_TX_CR);
	writel(IR_TX_IDC_H_VALUE, reg_addr + IR_TX_IDC_H);
	writel(IR_TX_IDC_L_VALUE, reg_addr + IR_TX_IDC_L);
	writel(IR_TX_STA_VALUE, reg_addr + IR_TX_STAR);
	writel(IR_TX_INT_C_VALUE, reg_addr + IR_TX_INTC);
	writel(IR_TX_GL_VALUE, reg_addr + IR_TX_GLR);

	return;
}

hal_irqreturn_t hal_cir_tx_handler(void *dev_id)
{
	unsigned int intsta;

	intsta = cir_tx_get_intsta();

	cir_tx_clr_intsta(intsta);

	return 0;
}

static int run_length_encode(unsigned int *raw_data, unsigned char *txbuf, unsigned int count)
{
	unsigned int is_high_level = 0;
	unsigned int num = 0;

	is_high_level = (*raw_data >> 24) & 0x01;

	num = ((*raw_data & 0x00FFFFFF) * 3) / three_pulse_cycle;
	while (num > 0x7f) {
		txbuf[count] = (is_high_level << 7) | 0x7f;
		count++;
		num -= 0x7f;
	}

	txbuf[count] = (is_high_level << 7) | num;
	count++;

	return count;
}

static int send_ir_code(void)
{
	unsigned int reg_val, i, idle_threshold;
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	reg_val = readl(reg_addr + IR_TX_GLR);
	reg_val |= 0x02;
	writel(reg_val, reg_addr + IR_TX_GLR);

	/* get idle threshold */
	idle_threshold = (readl(reg_addr + IR_TX_IDC_H) << 8) |
			readl(reg_addr + IR_TX_IDC_L);

	writel((cir_raw_buffer.tx_dcnt - 1), reg_addr + IR_TX_TR);

	if (cir_raw_buffer.tx_dcnt <= IR_TX_FIFO_SIZE) {
		for (i = 0; i < cir_raw_buffer.tx_dcnt; i++)
			writel(cir_raw_buffer.tx_buf[i], reg_addr + IR_TX_FIFO_DR);

	} else {
		printf("fail to packet\n");
		return -1;
	}

	reg_val = readl(reg_addr + IR_TX_TACR);
	printf("%3u bytes fifo available\n", reg_val);

	if (IR_TX_CYCLE_TYPE) {
		for (i = 0; i < cir_raw_buffer.tx_dcnt; i++)
			printf("%d, cir_raw_buffer code = 0x%x\n", i, cir_raw_buffer.tx_buf[i]);
		reg_val = readl(reg_addr + IR_TX_CR);
		reg_val |= (0x01 << 7);
		writel(reg_val, reg_addr + IR_TX_CR);
	} else {
		while (!cir_tx_fifo_empty()) {
			reg_val = readl(reg_addr + IR_TX_TACR);
		}
	}

	/* wait idle finish */
	while ((readl(reg_addr + IR_TX_ICR_H) << 8 | readl(reg_addr + IR_TX_ICR_L)) < idle_threshold) {
//		printf("wait idle\n");
	}

	printf("finish\n");

}

void hal_cir_tx_xmit(unsigned int *txbuf, unsigned int count)
{
	unsigned int *head_p, *data_p, *stop_p;
	unsigned int index = 0;
	int mark = 0;
	int i, ret, num;

	cir_tx_reset_rawbuffer();

	head_p = txbuf;

	*head_p |= (1 <<24);
	index = run_length_encode(head_p, cir_raw_buffer.tx_buf, index);
	head_p++;
	index = run_length_encode(head_p, cir_raw_buffer.tx_buf, index);

	data_p = (++head_p);

	num = count - 4;
	for (i = 0; i < num; i++) {
		if (mark == 1)
			mark = 0;
		else if (mark == 0) {
			*data_p |= (1 << 24);
			mark = 1;
		}
		index = run_length_encode(data_p, cir_raw_buffer.tx_buf, index);
		data_p++;
	}

	stop_p = data_p;

	index = run_length_encode(stop_p, cir_raw_buffer.tx_buf, index);
	stop_p++;
	*stop_p |= (1 << 24);
	index = run_length_encode(stop_p, cir_raw_buffer.tx_buf, index);

	cir_raw_buffer.tx_dcnt = index;

	send_ir_code();

	return;
}

void hal_cir_tx_set_carrier(int carrier_freq)
{
	unsigned int reg_val;
	unsigned int drmc = 1;
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	if ((carrier_freq > 6000000) || (carrier_freq < 15000)) {
		CIR_TX_ERR("invalid frequency of carrier: %d\n", carrier_freq);
		//return -1;
	}

	reg_val = readl(reg_addr + IR_TX_GLR);
	drmc = (reg_val >> 5) & 0x3;

	reg_val = IR_TX_CLK / ((2 + drmc) * carrier_freq) - 1;
	reg_val &= 0xff;
	writel(reg_val, reg_addr + IR_TX_MCR);

	return;
}

void hal_cir_tx_set_duty_cycle(int duty_cycle)
{
	unsigned int reg_val;
	unsigned long reg_addr = SUNXI_IRTX_PBASE;

	if (duty_cycle > 100) {
		CIR_TX_ERR("invalid duty_cycle: %d\n", duty_cycle);
		//return -1;
	}

	reg_val = readl(reg_addr + IR_TX_GLR);

	reg_val &= 0x9f;
	if (duty_cycle < 30) {
		reg_val |= 0x40;
		printf("set duty cycle to 25%%\n");
	} else if (duty_cycle < 40) {
		reg_val |= 0x20;
		printf("set duty cycle to 33%%\n");
	} else {
		printf("set duty cycle to 50%%\n");
	}

	writel(reg_val, reg_addr + IR_TX_GLR);
}

static int hal_cir_gpio_init(struct sunxi_cir_tx_t *cir_tx)
{
	cir_gpio_t *cir_tx_pin = cir_tx->pin;

	return hal_gpio_pinmux_set_function(cir_tx_pin->gpio, cir_tx_pin->enable_mux);
}

static void hal_cir_gpio_exit(struct sunxi_cir_tx_t *cir_tx)
{
	cir_gpio_t *cir_tx_pin = cir_tx->pin;

	hal_gpio_pinmux_set_function(cir_tx_pin->gpio, cir_tx_pin->disable_mux);
}
static int hal_cir_clk_init(struct sunxi_cir_tx_t *cir_tx)
{
	int ret = 0;

	hal_reset_type_t cir_tx_reset_type = SUNXI_IRTX_RESET_TYPE;
	hal_reset_id_t cir_tx_reset_id = SUNXI_RST_IRTX;

	cir_tx->cir_reset = hal_reset_control_get(cir_tx_reset_type, cir_tx_reset_id);
	if (hal_reset_control_reset(cir_tx->cir_reset)) {
		CIR_TX_ERR("cir reset  failed\n");
		ret = CIR_TX_CLK_ERR;
		goto err0;
	}

	cir_tx->cir_tx_clk_type = SUNXI_IRTX_CLK_TYPE;

	cir_tx->b_clk_id = SUNXI_CLK_BUS_IRTX;
	cir_tx->g_clk_id = SUNXI_CLK_MODULE_IRTX;

	if (cir_tx->b_clk_id) {
		cir_tx->bclk = hal_clock_get(cir_tx->cir_tx_clk_type, cir_tx->b_clk_id);
		if (hal_clock_enable(cir_tx->bclk)) {
			CIR_TX_ERR("cir_tx bclk enabled failed\n");
			ret = CIR_TX_CLK_ERR;
			goto err1;
		}
	}

	if (cir_tx->g_clk_id) {
		cir_tx->gclk = hal_clock_get(cir_tx->cir_tx_clk_type, cir_tx->g_clk_id);
		if (hal_clock_enable(cir_tx->gclk)) {
			CIR_TX_ERR("cir_tx gclk enabled failed\n");
			ret = CIR_TX_CLK_ERR;
			goto err2;
		}
	}

	return 0;

err2:
	if (hal_clock_is_enabled(cir_tx->bclk))
		hal_clock_disable(cir_tx->bclk);
err1:
	hal_reset_control_assert(cir_tx->cir_reset);
err0:
	return ret;
}

static void hal_cir_clk_exit(struct sunxi_cir_tx_t *cir_tx)
{
	if (hal_clock_is_enabled(cir_tx->gclk))
		hal_clock_disable(cir_tx->gclk);

	if (hal_clock_is_enabled(cir_tx->bclk))
		hal_clock_disable(cir_tx->bclk);

	hal_reset_control_assert(cir_tx->cir_reset);
}

cir_tx_status_t hal_cir_hw_init(struct sunxi_cir_tx_t *cir_tx)
{
	int err;

	if (hal_cir_clk_init(cir_tx)) {
		printf("cir_tx_clk_init err!\n");
		return CIR_TX_CLK_ERR;
	}

	if (hal_cir_gpio_init(cir_tx)) {
		printf("cir_tx_gpio_init err!\n");
		err = CIR_TX_PIN_ERR;
		goto err0;
	}

	hal_cir_reg_cfg();

	if (hal_request_irq(cir_tx->irq, hal_cir_tx_handler, "cir_tx", cir_tx) < 0) {
		printf("cir_tx request irq err\n");
		err = CIR_TX_IRQ_ERR;
		goto err1;
	}
	hal_enable_irq(cir_tx->irq);

	return CIR_TX_OK;

err1:
	hal_cir_gpio_exit(cir_tx);
err0:
	hal_cir_clk_exit(cir_tx);
	return err;
}

static void hal_cir_hw_exit(struct sunxi_cir_tx_t *cir_tx)
{
	hal_disable_irq(cir_tx->irq);
	hal_free_irq(cir_tx->irq);
	hal_cir_gpio_exit(cir_tx);
	hal_cir_clk_exit(cir_tx);
}

#ifdef CONFIG_COMPONENTS_PM
static inline void sunxi_irtx_save_regs(struct sunxi_cir_tx_t *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_irtx_regs_offset); i++)
		chip->regs_backup[i] = readl((unsigned long)(chip->reg_base) + sunxi_irtx_regs_offset[i]);
}

static inline void sunxi_irtx_restore_regs(struct sunxi_cir_tx_t *chip)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_irtx_regs_offset); i++)
		writel(chip->regs_backup[i], (unsigned long)(chip->reg_base) + sunxi_irtx_regs_offset[i]);
}

static int hal_cir_tx_resume(struct pm_device *dev, suspend_mode_t mode)
{
	struct sunxi_cir_tx_t *cir_tx = (struct sunxi_cir_tx_t *)dev->data;
	int ret = 0;

	if (hal_cir_clk_init(cir_tx)) {
		printf("cir_tx_clk_init err!\n");
		ret = CIR_TX_CLK_ERR;
		goto err0;
	}

	if (hal_cir_gpio_init(cir_tx)) {
		printf("cir_tx_gpio_init err!\n");
		ret = CIR_TX_PIN_ERR;
		goto err1;
	}

	hal_cir_reg_cfg();
	sunxi_irtx_restore_regs(cir_tx);
	hal_enable_irq(cir_tx->irq);
	CIR_TX_DBG("hal cir_tx resume\n");

	return 0;

err1:
	hal_cir_clk_exit(cir_tx);
err0:
	return ret;
}

static int hal_cir_tx_suspend(struct pm_device *dev, suspend_mode_t mode)
{
	struct sunxi_cir_tx_t *cir_tx = (struct sunxi_cir_tx_t *)dev->data;

	hal_disable_irq(cir_tx->irq);
	sunxi_irtx_save_regs(cir_tx);
	hal_cir_gpio_exit(cir_tx);
	hal_cir_clk_exit(cir_tx);
	CIR_TX_DBG("hal cir_tx suspend\n");

	return 0;
}

struct pm_devops pm_cir_tx_ops = {
	.suspend = hal_cir_tx_suspend,
	.resume = hal_cir_tx_resume,
};
#endif

cir_tx_status_t hal_cir_tx_init(struct sunxi_cir_tx_t *cir_tx)
{
	int ret;

	cir_tx = hal_malloc(128);

	cir_tx->reg_base = reg_base[0];
	cir_tx->irq = irq[0];
	cir_tx->pin = &pin[0];
	cir_tx->status = 1;

	ret = hal_cir_hw_init(cir_tx);
	if (ret) {
		CIR_TX_ERR("cir_tx hardware init error, ret:%d\n", ret);
		return ret;
	}

#ifdef CONFIG_COMPONENTS_PM
	cir_tx->pm.name = "cir_tx";
	cir_tx->pm.ops  = &pm_cir_tx_ops;
	cir_tx->pm.data = cir_tx;
	pm_devops_register(&cir_tx->pm);
#endif
	return ret;
}

cir_tx_status_t hal_cir_tx_deinit(struct sunxi_cir_tx_t *cir_tx)
{
#ifdef CONFIG_COMPONENTS_PM
	pm_devops_unregister(&cir_tx->pm);
#endif
	hal_cir_hw_exit(cir_tx);
	free(cir_tx);
}
