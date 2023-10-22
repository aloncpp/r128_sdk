/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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
#ifndef	__SUN55IW3_SPDIF_H_
#define	__SUN55IW3_SPDIF_H_

#define	SUNXI_SPDIF_MEMBASE (0x07116000)

/*add this to surport the spdif receive IEC-61937 data */
#define CONFIG_SND_SUNXI_SPDIF_RX_IEC61937

/*------------------SPDIF EXP register definition--------------------*/
#define	SUNXI_SPDIF_EXP_CTL	0x40
#define	SUNXI_SPDIF_EXP_ISTA	0x44
#define	SUNXI_SPDIF_EXP_INFO0	0x48
#define	SUNXI_SPDIF_EXP_INFO1	0x4C
#define	SUNXI_SPDIF_EXP_DBG0	0x50
#define	SUNXI_SPDIF_EXP_DBG1	0x54
#define	SUNXI_SPDIF_EXP_VER	0x58

/* SUNXI_SPDIF_EXP_CTL register */
#define INSET_DET_NUM		0
#define INSET_DET_EN		8
#define SYNCW_BIT_EN		9
#define DATA_TYPE_BIT_EN	10
#define DATA_LEG_BIT_EN		11
#define AUDIO_DATA_BIT_EN	12
#define RX_MODE			13
#define RX_MODE_MAN		14
#define UNIT_SEL		15
#define RPOTBF_NUM		16
#define BURST_DATA_OUT_SEL	30

/* SUNXI_SPDIF_EXP_ISTA register */
#define INSET_INT		0
#define PAPB_CAP_INT		1
#define PCPD_CAP_INT		2
#define RPDB_ERR_INT		3
#define PC_DTYOE_CH_INT		4
#define PC_ERR_FLAG_INT		5
#define PC_BIT_CH_INT		6
#define PC_PAUSE_STOP_INT	7
#define PD_CHAN_INT		8
#define INSET_INT_EN		16
#define PAPB_CAP_INT_EN		17
#define PCPD_CAP_INT_EN		18
#define RPDB_ERR_INT_EN		19
#define PC_DTYOE_CH_INT_EN	20
#define PC_ERR_FLAG_INT_EN	21
#define PC_BIT_CH_INT_EN	22
#define PC_PAUSE_STOP_INT_EN	23
#define PD_CHAN_INT_EN		24

/* SUNXI_SPDIF_EXP_INFO0 register */
#define PD_DATA_INFO		0
#define PC_DATA_INFO		16

/* SUNXI_SPDIF_EXP_INFO1 register */
#define SAMPLE_RATE_VAL		0
#define RPOTBF_VAL		16

/* SUNXI_SPDIF_EXP_DBG0 register */
#define RE_DATA_COUNT_VAL	0
#define DATA_CAP_STA_MACHE	16

/* SUNXI_SPDIF_EXP_DBG1 register */
#define SAMPLE_RATE_COUNT	0
#define RPOTBF_COUNT		16

/* SUNXI_SPDIF_EXP_VER register */
#define MOD_VER			0

/*------------------------ PIN CONFIG FOR NORMAL ---------------------------*/
spdif_gpio_t g_spdif_gpio = {
	/* .clk	= {GPIOB(12), 3}, */
	//.out	= {GPIOA(17), 3},	/* use for uart0 rx */
	//.in	= {GPIOA(16), 3},	/* use for uart0 tx */
	.out	= {GPIOB(6), 3},
	.in	= {GPIOB(7), 3},
};

/*------------------------ CLK CONFIG FOR SUN20IW2 ---------------------------*/
struct sunxi_spdif_clk {
	struct reset_control *clk_rst;		/* RESET: RST_SPDIF */
	hal_clk_t clk_bus;			/* CCU: CLK_BUS_SPDIF */
};

static inline int snd_sunxi_spdif_clk_enable(struct sunxi_spdif_clk *clk)
{
	int ret;

	snd_print("\n");

	/* rst & bus */
	ret = hal_reset_control_deassert(clk->clk_rst);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("spdif clk_rst clk_deassert failed.\n");
		goto err_deassert_clk_rst;
	}
	ret = hal_clock_enable(clk->clk_bus);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("spdif clk_bus enable failed.\n");
		goto err_enable_clk_bus;
	}

	return HAL_CLK_STATUS_OK;

err_enable_clk_bus:
	hal_reset_control_assert(clk->clk_rst);
err_deassert_clk_rst:
	return HAL_CLK_STATUS_ERROR;
}

static inline void snd_sunxi_spdif_clk_disable(struct sunxi_spdif_clk *clk)
{
	snd_print("\n");

	hal_clock_disable(clk->clk_bus);
	hal_reset_control_assert(clk->clk_rst);

	return;
}

static inline int snd_sunxi_spdif_clk_init(struct sunxi_spdif_clk *clk)
{
	int ret;

	snd_print("\n");

	/* rst & bus */
	clk->clk_rst = hal_reset_control_get(HAL_SUNXI_DSP_RESET, RST_BUS_DSP_SPDIF);
	if (!clk->clk_rst) {
		snd_err("spdif clk_rst hal_reset_control_get failed\n");
		goto err_get_clk_rst;
	}
	clk->clk_bus = hal_clock_get(HAL_SUNXI_DSP, CLK_BUS_DSP_SPDIF);
	if (!clk->clk_bus) {
		snd_err("spdif clk_bus hal_clock_get failed\n");
		goto err_get_clk_bus;
	}

	/* note: Enable and then set the freq to avoid clock lock errors */
	ret = snd_sunxi_spdif_clk_enable(clk);
	if (ret != HAL_CLK_STATUS_OK) {
		snd_err("spdif snd_sunxi_spdif_clk_enable failed.\n");
		goto err_clk_enable;
	}

	return HAL_CLK_STATUS_OK;

err_clk_enable:
	hal_clock_put(clk->clk_bus);
err_get_clk_bus:
	hal_reset_control_put(clk->clk_rst);
err_get_clk_rst:
	return HAL_CLK_STATUS_ERROR;
}

static inline void snd_sunxi_spdif_clk_exit(struct sunxi_spdif_clk *clk)
{
	snd_print("\n");

	snd_sunxi_spdif_clk_disable(clk);
	hal_clock_put(clk->clk_bus);
	hal_reset_control_put(clk->clk_rst);

	return;
}

static inline int snd_sunxi_spdif_clk_set_rate(struct sunxi_spdif_clk *clk, int stream,
					       unsigned int freq_in, unsigned int freq_out)
{
	return HAL_CLK_STATUS_OK;
}

#endif /* __SUN55IW3_SPDIF_H_ */
