/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.


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
#include <hal_interrupt.h>
#include <sunxi_hal_gpadc.h>
#include <hal_time.h>
#include <hal_timer.h>
#ifdef CONFIG_COMPONENTS_PM
#include <pm_devops.h>
#include <pm_wakesrc.h>
#include <pm_wakecnt.h>
#endif

hal_gpadc_t hal_gpadc;

#if defined(CONFIG_SOC_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20)
static hal_gpadc_status_t hal_gpadc_clk_init(hal_gpadc_t *gpadc)
{
    hal_clk_type_t clk_type = HAL_SUNXI_CCU;
    hal_clk_id_t gpadc_clk_id = gpadc->bus_clk;

    hal_reset_type_t reset_type = HAL_SUNXI_RESET;
    hal_reset_id_t gpadc_reset_id = gpadc->rst_clk;

    gpadc->reset = hal_reset_control_get(reset_type, gpadc_reset_id);
    if (hal_reset_control_reset(gpadc->reset))
    {
        GPADC_ERR("gpadc reset deassert failed!\n");
        return GPADC_ERROR;
    }

    gpadc->mbus_clk = hal_clock_get(clk_type, gpadc_clk_id);
    if (hal_clock_enable(gpadc->mbus_clk))
    {
        GPADC_ERR("gpadc clk enable failed!\n");
        goto err0;
    }

    return GPADC_OK;

err0:
    hal_reset_control_assert(gpadc->reset);
    hal_reset_control_put(gpadc->reset);
    return GPADC_ERROR;
}
#elif defined(CONFIG_ARCH_SUN20IW2)
static hal_gpadc_status_t hal_gpadc_clk_init(hal_gpadc_t *gpadc)
{
    hal_clk_type_t clk_type = HAL_SUNXI_AON_CCU;
    hal_clk_id_t gpadc_clk_id = gpadc->bus_clk;

    hal_clk_id_t gpadc_clk_id1 = CLK_GPADC_CTRL;

    hal_reset_type_t reset_type = HAL_SUNXI_AON_RESET;
    hal_reset_id_t gpadc_reset_id = gpadc->rst_clk;

    hal_clk_t pclk;

    gpadc->reset = hal_reset_control_get(reset_type, gpadc_reset_id);
    if (hal_reset_control_reset(gpadc->reset))
    {
        GPADC_ERR("gpadc reset deassert failed!\n");
        return GPADC_ERROR;
    }

    gpadc->mbus_clk = hal_clock_get(clk_type, gpadc_clk_id);
    if (hal_clock_enable(gpadc->mbus_clk))
    {
        GPADC_ERR("gpadc clk enable failed!\n");
        goto err0;
    }

    gpadc->mbus_clk1 = hal_clock_get(clk_type, gpadc_clk_id1);
    if (hal_clock_enable(gpadc->mbus_clk1))
    {
        GPADC_ERR("gpadc clk1 enable failed!\n");
        goto err1;
    }

    pclk = hal_clock_get(clk_type, CLK_HOSC);
    hal_clk_set_parent(gpadc->mbus_clk1, pclk);

    hal_clk_set_rate(gpadc->mbus_clk1, GPADC_CLOCK);
    /* release after use */
    hal_clock_put(pclk);

    return GPADC_OK;

err1:
    hal_clock_disable(gpadc->mbus_clk);
    hal_clock_put(gpadc->mbus_clk);
err0:
    hal_reset_control_assert(gpadc->reset);
    hal_reset_control_put(gpadc->reset);
    return GPADC_ERROR;
}
#elif defined(CONFIG_ARCH_SUN20IW3)
static hal_gpadc_status_t hal_gpadc_clk_init(hal_gpadc_t *gpadc)
{
    hal_clk_type_t clk_type = HAL_SUNXI_CCU;
    hal_clk_id_t gpadc_clk_id = HAL_CLK_PERIPH_GPADC;

    gpadc->mclk = hal_clock_get(clk_type, gpadc_clk_id);
    if(hal_clock_enable(gpadc->mclk))
    {
        GPADC_ERR("gpadc clk enable failed!\n");
        return GPADC_ERROR;
    }

    return GPADC_OK;
}
#else
static hal_gpadc_status_t hal_gpadc_clk_init(hal_gpadc_t *gpadc)
{
#if !defined(CONFIG_ARCH_SUN8IW18P1)
    if (hal_clk_set_parent(gpadc->mclk, gpadc->pclk))
    {
        GPADC_ERR("[gpadc] clk set parent failed!");
        return GPADC_ERROR;
    }
#endif
    if (hal_clock_enable(gpadc->mclk))
    {
        GPADC_ERR("[gpadc] clk enable mclk failed!");
        return GPADC_ERROR;
    }

    return GPADC_OK;
}
#endif

static void hal_gpadc_clk_exit(hal_gpadc_t *gpadc)
{
#if defined(CONFIG_SOC_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20)
    hal_clock_disable(gpadc->mbus_clk);
    hal_clock_put(gpadc->mbus_clk);
    hal_reset_control_assert(gpadc->reset);
    hal_reset_control_put(gpadc->reset);
#elif defined(CONFIG_ARCH_SUN20IW2)
    hal_clock_disable(gpadc->mbus_clk1);
    hal_clock_put(gpadc->mbus_clk1);
    hal_clock_disable(gpadc->mbus_clk);
    hal_clock_put(gpadc->mbus_clk);
    hal_reset_control_assert(gpadc->reset);
    hal_reset_control_put(gpadc->reset);
#else
    hal_clock_disable(gpadc->mclk);
    hal_clock_put(gpadc->mclk);
#endif
}

static int gpadc_channel_check_valid(hal_gpadc_channel_t channal)
{
    hal_gpadc_t *gpadc = &hal_gpadc;

#if defined(CONFIG_ARCH_SUN20IW2)
    if (channal == 12)
        return 0;
#endif

    return channal < gpadc->channel_num ? 0 : -1 ;
}

static void gpadc_channel_select(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CS_EN_REG);
    reg_val |= (0x01 << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CS_EN_REG);

}

static void gpadc_channel_deselect(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CS_EN_REG);
    reg_val &= ~(0x01 << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CS_EN_REG);

}

static void gpadc_compare_select(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CS_EN_REG);
    reg_val |= (GP_CH0_CMP_EN << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CS_EN_REG);

}

static void gpadc_compare_deselect(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CTRL_REG);
    reg_val &= ~(GP_CH0_CMP_EN << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CTRL_REG);

}

void gpadc_channel_enable_lowirq(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_DATAL_INTC_REG);
    reg_val |= (0x01 << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_DATAL_INTC_REG);
}

static void gpadc_channel_disable_lowirq(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_DATAL_INTC_REG);
    reg_val &= ~(0x01 << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_DATAL_INTC_REG);
}

static void gpadc_channel_compare_lowdata(hal_gpadc_channel_t channal,
        uint32_t low_uv)
{
    uint32_t reg_val = 0, low = 0, unit = 0;
    hal_gpadc_t *gpadc = &hal_gpadc;

    /* analog voltage range 0~1.8v, 12bits sample rate, unit=1.8v/(2^12) */
    unit = VOL_RANGE / 4096; /* 12bits sample rate */
    low = low_uv / unit;
    if (low > VOL_VALUE_MASK)
    {
        low = VOL_VALUE_MASK;
    }

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CH0_CMP_DATA_REG + 4 * channal);
    reg_val &= ~VOL_VALUE_MASK;
    reg_val |= (low & VOL_VALUE_MASK);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CH0_CMP_DATA_REG + 4 * channal);

}

void gpadc_channel_enable_highirq(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_DATAH_INTC_REG);
    reg_val |= (1 << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_DATAH_INTC_REG);

}

void gpadc_key_enable_highirq(hal_gpadc_channel_t channal)
{
    gpadc_channel_enable_highirq(channal);
}

void gpadc_channel_disable_highirq(hal_gpadc_channel_t channal)
{
    uint32_t reg_val;
    hal_gpadc_t *gpadc = &hal_gpadc;

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_DATAH_INTC_REG);
    reg_val &= ~(1 << channal);
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_DATAH_INTC_REG);

}

void gpadc_key_disable_highirq(hal_gpadc_channel_t channal)
{
    gpadc_channel_disable_highirq(channal);
}

static void gpadc_channel_compare_highdata(hal_gpadc_channel_t channal,
        uint32_t hig_uv)
{
    uint32_t reg_val = 0, hig_val = 0, unit_val = 0;
    hal_gpadc_t *gpadc = &hal_gpadc;

    /* anolog voltage range 0~1.8v, 12bits sample rate, unit=1.8v/(2^12) */
    unit_val = VOL_RANGE / 4096; /* 12bits sample rate */
    hig_val = hig_uv / unit_val;

    if (hig_val > VOL_VALUE_MASK)
    {
        hig_val = VOL_VALUE_MASK;
    }

    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CH0_CMP_DATA_REG + 4 * channal);
    reg_val &= ~(VOL_VALUE_MASK << 16);
    reg_val |= (hig_val & VOL_VALUE_MASK) << 16;
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CH0_CMP_DATA_REG + 4 * channal);

}

/* clk_in: source clock, round_clk: sample rate */
static void gpadc_sample_rate_set(uint32_t reg_base, uint32_t clk_in,
                                  uint32_t round_clk)
{
    uint32_t div, reg_val;
    if (round_clk > clk_in)
    {
        GPADC_ERR("invalid round clk!");
    }
    div = clk_in / round_clk - 1 ;
    reg_val = readl((unsigned long)(reg_base) + GP_SR_REG);
    reg_val &= ~GP_SR_CON;
    reg_val |= (div << 16);
    writel(reg_val, (unsigned long)(reg_base) + GP_SR_REG);

#if defined(CONFIG_ARCH_SUN20IW2)
    uint32_t tacq;

    tacq = clk_in / DEFAULT_SR / 10 - 1;
    reg_val = readl((unsigned long)(reg_base) + GP_SR_REG);
    reg_val &= ~GP_SR_TACQ;
    reg_val |= tacq;
    writel(reg_val, (unsigned long)(reg_base) + GP_SR_REG);
#endif
}

static void gpadc_calibration_enable(uint32_t reg_base)
{
    uint32_t reg_val;
    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val |= GP_CALI_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

static void gpadc_mode_select(uint32_t reg_base,
                              enum gp_select_mode mode)
{
    uint32_t reg_val;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val &= ~GP_MODE_SELECT;
    reg_val |= (mode << 18);
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

#if defined(CONFIG_ARCH_SUN20IW2)
static void gpadc_datairq_enable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_FIFO_INTC_REG);
    reg_val |= FIFO_DATA_IRQ_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_FIFO_INTC_REG);
}

static void gpadc_datairq_disable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_FIFO_INTC_REG);
    reg_val &= ~FIFO_DATA_IRQ_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_FIFO_INTC_REG);
}

static void gpadc_vbat_det_enable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val |= GP_VBAT_DET_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

static void gpadc_vbat_det_disable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val &= ~GP_VBAT_DET_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

static void gpadc_vref_mode_select(uint32_t reg_base, u32 flag)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val &= ~GP_VREF_MODE_SEL;
    reg_val |= flag << 1;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

/* enable gpadc ldo function, true:enable, false:disable */
static void gpadc_ldo_enable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val |= GP_ADC_LDO_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

static void gpadc_ldo_disable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val &= ~GP_ADC_LDO_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}
#endif /* CONFIG_ARCH_SUN20IW2 */

/* enable gpadc function, true:enable, false:disable */
static void gpadc_enable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val |= GP_ADC_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

/* enable gpadc function, true:enable, false:disable */
static void gpadc_disable(uint32_t reg_base)
{
    uint32_t reg_val = 0;

    reg_val = readl((unsigned long)(reg_base) + GP_CTRL_REG);
    reg_val &= ~GP_ADC_EN;
    writel(reg_val, (unsigned long)(reg_base) + GP_CTRL_REG);
}

static uint32_t gpadc_read_channel_irq_enable(uint32_t reg_base)
{
    return readl((unsigned long)(reg_base) + GP_DATA_INTC_REG);
}

static uint32_t gpadc_read_channel_lowirq_enable(uint32_t reg_base)
{
    return readl((unsigned long)(reg_base) + GP_DATAL_INTC_REG);
}

static uint32_t gpadc_read_channel_highirq_enable(uint32_t reg_base)
{
    return readl((unsigned long)(reg_base) + GP_DATAH_INTC_REG);
}

static uint32_t gpadc_channel_irq_status(uint32_t reg_base)
{
    return readl((unsigned long)(reg_base) + GP_DATA_INTS_REG);
}

static void gpadc_channel_clear_irq(uint32_t reg_base, uint32_t flags)
{
    writel(flags, (unsigned long)(reg_base) + GP_DATA_INTS_REG);
}

static uint32_t gpadc_channel_lowirq_status(uint32_t reg_base)
{
    return readl((unsigned long)(reg_base) + GP_DATAL_INTS_REG);
}

static void gpadc_channel_clear_lowirq(uint32_t reg_base, uint32_t flags)
{
    writel(flags, (unsigned long)(reg_base) + GP_DATAL_INTS_REG);
}

static uint32_t gpadc_channel_highirq_status(uint32_t reg_base)
{
    return readl((unsigned long)(reg_base) + GP_DATAH_INTS_REG);
}

static void gpadc_channel_clear_highirq(uint32_t reg_base, uint32_t flags)
{
    writel(flags, (unsigned long)(reg_base) + GP_DATAH_INTS_REG);
}

static int gpadc_read_data(uint32_t reg_base, hal_gpadc_channel_t channal)
{
    return readl((unsigned long)(reg_base) + GP_CH0_DATA_REG + 4 * channal) & GP_CH_DATA_MASK;
}

uint32_t gpadc_read_channel_data(hal_gpadc_channel_t channel)
{
    int data;
    uint32_t vol_data;
    hal_gpadc_t *gpadc = &hal_gpadc;

    data = gpadc_read_data(gpadc->reg_base, channel);
    data = ((VOL_RANGE / 4096) * data);
    vol_data = data / 1000;
    GPADC_INFO("channel %d vol data: %u\n", channel, vol_data);

    return vol_data;
}

int hal_gpadc_callback(uint32_t data_type, uint32_t data)
{
    GPADC_INFO("gpadc interrupt, data_type is %u", data_type);
    return 0;
}

static hal_irqreturn_t gpadc_handler(void *dev)
{
    hal_gpadc_t *gpadc = (hal_gpadc_t *)dev;

    uint32_t reg_val, reg_low, reg_high;
    uint32_t reg_enable, reg_enable_low, reg_enable_high;
    uint32_t i, data = 0;

    reg_enable = gpadc_read_channel_irq_enable(gpadc->reg_base);
    reg_enable_low = gpadc_read_channel_lowirq_enable(gpadc->reg_base);
    reg_enable_high = gpadc_read_channel_highirq_enable(gpadc->reg_base);

    reg_val = gpadc_channel_irq_status(gpadc->reg_base);
    gpadc_channel_clear_irq(gpadc->reg_base, reg_val);
    reg_low = gpadc_channel_lowirq_status(gpadc->reg_base);
    gpadc_channel_clear_lowirq(gpadc->reg_base, reg_low);
    reg_high = gpadc_channel_highirq_status(gpadc->reg_base);
    gpadc_channel_clear_highirq(gpadc->reg_base, reg_high);

    for (i = 0; i < gpadc->channel_num; i++)
    {
        if (reg_low & (1 << i) & reg_enable_low)
        {
            data = gpadc_read_data(gpadc->reg_base, i);
            //gpadc_channel_enable_highirq(i);

            if (gpadc->callback[i])
            {
                gpadc->callback[i](GPADC_DOWN, data);
            }
        }

        if (reg_high & (1 << i) & reg_enable_high)
        {
            data = gpadc_read_data(gpadc->reg_base, i);
            //gpadc_channel_disable_highirq(i);
            gpadc->callback[i](GPADC_UP, data);
        }
    }

    return 0;
}

hal_gpadc_status_t hal_gpadc_register_callback(hal_gpadc_channel_t channal,
        gpadc_callback_t user_callback)
{
    hal_gpadc_t *gpadc = &hal_gpadc;

    if (gpadc_channel_check_valid(channal))
    {
        return GPADC_CHANNEL_ERROR;
    }

    if (user_callback == NULL)
    {
        return GPADC_ERROR;
    }

    gpadc->callback[channal] = user_callback;

    return GPADC_OK;
}

hal_gpadc_status_t hal_gpadc_channel_init(hal_gpadc_channel_t channal)
{
    hal_gpadc_t *gpadc = &hal_gpadc;

    if (gpadc_channel_check_valid(channal))
    {
        return GPADC_CHANNEL_ERROR;
    }

    gpadc_channel_select(channal);
    gpadc_compare_select(channal);
    gpadc_channel_enable_lowirq(channal);

#if defined(CONFIG_ARCH_SUN20IW2)
    if (channal == GP_CH_8)
    {
        gpadc_channel_compare_lowdata(channal, POWER_PROTECT_LOWDATA);
        gpadc_channel_compare_highdata(channal, POWER_PROTECT_HIGDATA);
    } else {
        gpadc_channel_compare_lowdata(channal, COMPARE_LOWDATA);
        gpadc_channel_compare_highdata(channal, COMPARE_HIGDATA);
    }
#else
    gpadc_channel_compare_lowdata(channal, COMPARE_LOWDATA);
    gpadc_channel_compare_highdata(channal, COMPARE_HIGDATA);

#endif

    hal_msleep(4);
    return GPADC_OK;
}

hal_gpadc_status_t hal_gpadc_channel_exit(hal_gpadc_channel_t channal)
{
    hal_gpadc_t *gpadc = &hal_gpadc;

    if (gpadc_channel_check_valid(channal))
    {
        return GPADC_CHANNEL_ERROR;
    }

    gpadc_channel_deselect(channal);
    gpadc_compare_deselect(channal);
    gpadc_channel_disable_lowirq(channal);

    return GPADC_OK;
}

static int hal_gpadc_setup(hal_gpadc_t *gpadc)
{
    uint8_t i;

    gpadc->reg_base = GPADC_BASE;
    gpadc->channel_num = CHANNEL_NUM;
    gpadc->irq_num = SUNXI_GPADC_IRQ;
    gpadc->sample_rate = DEFAULT_SR;
#if defined(CONFIG_SOC_SUN20IW1) || defined(CONFIG_ARCH_SUN8IW20)
    gpadc->bus_clk = CLK_BUS_GPADC;
    gpadc->rst_clk = RST_BUS_GPADC;
#elif defined(CONFIG_ARCH_SUN20IW2)
    gpadc->bus_clk = CLK_GPADC;
    gpadc->rst_clk = RST_GPADC;
#else
    gpadc->pclk = HAL_CLK_SRC_HOSC24M;
    gpadc->mclk = HAL_CLK_PERIPH_GPADC;
#endif
    gpadc->mode = GP_CONTINUOUS_MODE;

    for (i = 0; i < gpadc->channel_num; i++)
    {
        gpadc->callback[i] = hal_gpadc_callback;
    }

    if (hal_request_irq(gpadc->irq_num, gpadc_handler, "gpadc", gpadc) < 0)
    {
        return GPADC_IRQ_ERROR;
    }

    return GPADC_OK;
};

static int hal_gpadc_hw_init(hal_gpadc_t *gpadc)
{
#if defined(CONFIG_ARCH_SUN20IW2)
    unsigned long rate;
#endif

    if (hal_gpadc_clk_init(gpadc))
    {
        GPADC_ERR("gpadc init clk error");
        return GPADC_CLK_ERROR;
    }

    GPADC_INFO("gpadc set sample rate");
#if defined(CONFIG_ARCH_SUN20IW2)
    rate = hal_clk_get_rate(gpadc->mbus_clk1);
    gpadc_sample_rate_set(gpadc->reg_base, rate, gpadc->sample_rate);
#else
    gpadc_sample_rate_set(gpadc->reg_base, OSC_24MHZ, gpadc->sample_rate);
#endif
    GPADC_INFO("gpadc enable calibration");
    gpadc_calibration_enable(gpadc->reg_base);
    gpadc_mode_select(gpadc->reg_base, gpadc->mode);

#if defined(CONFIG_ARCH_SUN20IW2)
    gpadc_vbat_det_enable(gpadc->reg_base);
    gpadc_vref_mode_select(gpadc->reg_base, 0x1);
    gpadc_ldo_enable(gpadc->reg_base);
    hal_usleep(100);
#endif

    gpadc_enable(gpadc->reg_base);

    hal_enable_irq(gpadc->irq_num);

    return GPADC_OK;
}

static void hal_gpadc_hw_exit(hal_gpadc_t *gpadc)
{
    hal_disable_irq(gpadc->irq_num);
    gpadc_disable(gpadc->reg_base);
    hal_gpadc_clk_exit(gpadc);
}

#ifdef CONFIG_COMPONENTS_PM
static inline void hal_gpadc_save_regs(hal_gpadc_t *gpadc)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(hal_gpadc_regs_offset); i++)
        gpadc->regs_backup[i] = readl((unsigned long)(gpadc->reg_base) + hal_gpadc_regs_offset[i]);
}

static inline void hal_gpadc_restore_regs(hal_gpadc_t *gpadc)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(hal_gpadc_regs_offset); i++)
        writel(gpadc->regs_backup[i], (unsigned long)(gpadc->reg_base) + hal_gpadc_regs_offset[i]);
}

void hal_gpadc_report_wakeup_event()
{
	pm_wakecnt_inc(SUNXI_GPADC_IRQ);
}

static int hal_gpadc_resume(struct pm_device *dev, suspend_mode_t mode)
{
    hal_gpadc_t *gpadc = &hal_gpadc;
    hal_clk_t pclk;

#if ! (defined(CONFIG_PM_WAKESRC_GPADC) && defined(CONFIG_ARCH_SUN20IW2))
    /* when gpadc as wakeup src, don't need enable hardware in resume */
    hal_gpadc_hw_init(gpadc);
#else
    /* change clk to CLK_HOSC when resume */
    pclk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_HOSC);
    hal_clk_set_parent(gpadc->mbus_clk1, pclk);

    /* release after use */
    hal_clock_put(pclk);
#endif

    hal_gpadc_restore_regs(gpadc);
    GPADC_INFO("hal gpadc resume\n");
    return 0;
}

static int hal_gpadc_suspend(struct pm_device *dev, suspend_mode_t mode)
{
    hal_gpadc_t *gpadc = &hal_gpadc;
    u32 reg_val;
    hal_clk_t pclk;

#if defined(CONFIG_PM_WAKESRC_GPADC) && defined(CONFIG_ARCH_SUN20IW2)
    /* Device should not suspend when acting as a wakeup source.
     * If necessary, dO some wakeup preparation instead.
     */
    hal_gpadc_save_regs(gpadc);

    /* change clk to 32k clk_src when sleep or standby */
    pclk = hal_clock_get(HAL_SUNXI_FIXED_CCU, CLK_SRC_LOSC);
    hal_clk_set_parent(gpadc->mbus_clk1, pclk);

    /* release after use */
    hal_clock_put(pclk);

    /* change gpadc to low power mode when sleep or stangby */
    reg_val = readl((unsigned long)(gpadc->reg_base) + GP_CTRL_REG);
    reg_val |= GP_LP_EN;
    writel(reg_val, (unsigned long)(gpadc->reg_base) + GP_CTRL_REG);

    GPADC_INFO("hal gpadc wakesrc suspend\n");
#else
    hal_gpadc_save_regs(gpadc);
    hal_gpadc_hw_exit(gpadc);

    GPADC_INFO("hal gpadc suspend\n");
#endif

    return 0;
}

struct pm_devops pm_gpadc_ops = {
    .suspend = hal_gpadc_suspend,
    .resume  = hal_gpadc_resume,
};

struct pm_device pm_gpadc = {
    .name = "sunxi_gpadc",
    .ops  = &pm_gpadc_ops,
};
#endif

int hal_gpadc_init(void)
{
    hal_gpadc_t *gpadc = &hal_gpadc;
    int err;

    if (gpadc->already_init) {
        gpadc->already_init++;
        GPADC_INFO("gpadc has been inited, return ok\n");
        return GPADC_OK;
    }

    err = hal_gpadc_setup(gpadc);
    if (err)
    {
        GPADC_ERR("gpadc setup failed\n");
        return GPADC_ERROR;
    }

    err = hal_gpadc_hw_init(gpadc);
    if (err)
    {
        GPADC_ERR("gpadc init hw failed\n");
        return GPADC_ERROR;
    }

#ifdef CONFIG_COMPONENTS_PM
    if (pm_wakesrc_register(gpadc->irq_num, "gpadc", PM_WAKESRC_MIGHT_WAKEUP)) {
        GPADC_ERR("gpadc registers wakesrc fail\n");
        return GPADC_ERROR;
    }
    if (pm_set_wakeirq(gpadc->irq_num)) {
        GPADC_ERR("gpadc pm_set_wakeirq fail\n");
        return GPADC_ERROR;
    }

    pm_devops_register(&pm_gpadc);
#endif

    gpadc->already_init++;
    GPADC_INFO("gpadc init success\n");
    return GPADC_OK;
}

hal_gpadc_status_t hal_gpadc_deinit(void)
{
    hal_gpadc_t *gpadc = &hal_gpadc;

    if (gpadc->already_init > 0) {
        if (--gpadc->already_init == 0) {
#ifdef CONFIG_COMPONENTS_PM
            pm_devops_unregister(&pm_gpadc);
            if (pm_clear_wakeirq(gpadc->irq_num)) {
                GPADC_ERR("gpadc pm_clear_wakeirq fail\n");
                return GPADC_ERROR;
            }
            if (pm_wakesrc_unregister(gpadc->irq_num)) {
                GPADC_ERR("gpadc unregisters wakesrc fail\n");
                return GPADC_ERROR;
            }
#endif
            hal_gpadc_hw_exit(gpadc);
	}
    }

    return GPADC_OK;
}
