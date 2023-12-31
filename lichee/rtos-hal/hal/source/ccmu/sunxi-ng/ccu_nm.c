// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */
#include "ccu.h"
#include "ccu_frac.h"
#include "ccu_gate.h"
#include "ccu_nm.h"

struct _ccu_nm
{
    unsigned long   n, min_n, max_n;
    unsigned long   m, min_m, max_m;
};

static unsigned long ccu_nm_calc_rate(unsigned long parent,
                                      unsigned long n, unsigned long m)
{
    u64 rate = parent;

    rate *= n;
    rate /= m;

    return rate;
}

static void ccu_nm_find_best(unsigned long parent, unsigned long rate,
                             struct _ccu_nm *nm)
{
    unsigned long best_rate = 0;
    unsigned long best_n = 0, best_m = 0;
    unsigned long _n, _m;

    for (_n = nm->min_n; _n <= nm->max_n; _n++)
    {
        for (_m = nm->min_m; _m <= nm->max_m; _m++)
        {
            unsigned long tmp_rate = ccu_nm_calc_rate(parent,
                                     _n, _m);

            if (tmp_rate > rate)
            {
                continue;
            }

            if ((rate - tmp_rate) < (rate - best_rate))
            {
                best_rate = tmp_rate;
                best_n = _n;
                best_m = _m;
            }
        }
    }

    nm->n = best_n;
    nm->m = best_m;
}

static void ccu_nm_disable(struct clk_hw *hw)
{
    struct ccu_nm *nm = hw_to_ccu_nm(hw);

    return ccu_gate_helper_disable(&nm->common, nm->enable);
}

static int ccu_nm_enable(struct clk_hw *hw)
{
    struct ccu_nm *nm = hw_to_ccu_nm(hw);

    return ccu_gate_helper_enable(&nm->common, nm->enable);
}

static int ccu_nm_is_enabled(struct clk_hw *hw)
{
    struct ccu_nm *nm = hw_to_ccu_nm(hw);

    return ccu_gate_helper_is_enabled(&nm->common, nm->enable);
}

static unsigned long ccu_nm_recalc_rate(struct clk_hw *hw,
                                        unsigned long parent_rate)
{
    struct ccu_nm *nm = hw_to_ccu_nm(hw);
    unsigned long rate;
    unsigned long n, m;
    u32 reg;

    if (ccu_frac_helper_is_enabled(&nm->common, &nm->frac))
    {
        rate = ccu_frac_helper_read_rate(&nm->common, &nm->frac);

        if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
        {
            rate /= nm->fixed_post_div;
        }

        return rate;
    }

    reg = readl(nm->common.base + nm->common.reg);

    n = reg >> nm->n.shift;
    n &= (1 << nm->n.width) - 1;
    n += nm->n.offset;
    if (!n)
    {
        n++;
    }

    m = reg >> nm->m.shift;
    m &= (1 << nm->m.width) - 1;
    m += nm->m.offset;
    if (!m)
    {
        m++;
    }

    if (ccu_sdm_helper_is_enabled(&nm->common, &nm->sdm))
    {
        rate = ccu_sdm_helper_read_rate(&nm->common, &nm->sdm, m, n);
    }
    else
    {
        rate = ccu_nm_calc_rate(parent_rate, n, m);
    }

    if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate /= nm->fixed_post_div;
    }

    return rate;
}

static unsigned long ccu_pll12_nm_recalc_rate(struct clk_hw *hw,
		unsigned long parent_rate)
{
	return 1920000000;
}

static long ccu_nm_round_rate(struct clk_hw *hw, unsigned long rate,
                              unsigned long *parent_rate)
{
    struct ccu_nm *nm = hw_to_ccu_nm(hw);
    struct _ccu_nm _nm;

    if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate *= nm->fixed_post_div;
    }

    if (rate < nm->min_rate)
    {
        rate = nm->min_rate;
        if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
        {
            rate /= nm->fixed_post_div;
        }
        return rate;
    }

    if (nm->max_rate && rate > nm->max_rate)
    {
        rate = nm->max_rate;
        if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
        {
            rate /= nm->fixed_post_div;
        }
        return rate;
    }

    if (ccu_frac_helper_has_rate(&nm->common, &nm->frac, rate))
    {
        if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
        {
            rate /= nm->fixed_post_div;
        }
        return rate;
    }

    if (ccu_sdm_helper_has_rate(&nm->common, &nm->sdm, rate))
    {
        if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
        {
            rate /= nm->fixed_post_div;
        }
        return rate;
    }

    _nm.min_n = nm->n.min ? : 1;
    _nm.max_n = nm->n.max ? : 1 << nm->n.width;
    _nm.min_m = 1;
    _nm.max_m = nm->m.max ? : 1 << nm->m.width;

    ccu_nm_find_best(*parent_rate, rate, &_nm);
    rate = ccu_nm_calc_rate(*parent_rate, _nm.n, _nm.m);

    if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate /= nm->fixed_post_div;
    }

    return rate;
}

static int ccu_nm_set_rate(struct clk_hw *hw, unsigned long rate,
                           unsigned long parent_rate)
{
    struct ccu_nm *nm = hw_to_ccu_nm(hw);
    struct _ccu_nm _nm;
    u32 reg;
    u32 __cspr;

    /* Adjust target rate according to post-dividers */
    if (nm->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate = rate * nm->fixed_post_div;
    }

    if (ccu_frac_helper_has_rate(&nm->common, &nm->frac, rate))
    {
        __cspr = hal_spin_lock_irqsave(&nm->common.lock);

        /* most SoCs require M to be 0 if fractional mode is used */
        reg = readl(nm->common.base + nm->common.reg);
        reg &= ~GENMASK(nm->m.width + nm->m.shift - 1, nm->m.shift);
        writel(reg, nm->common.base + nm->common.reg);

        hal_spin_unlock_irqrestore(&nm->common.lock, __cspr);

        ccu_frac_helper_enable(&nm->common, &nm->frac);

        return ccu_frac_helper_set_rate(&nm->common, &nm->frac,
                                        rate, nm->lock);
    }
    else
    {
        ccu_frac_helper_disable(&nm->common, &nm->frac);
    }

    _nm.min_n = nm->n.min ? : 1;
    _nm.max_n = nm->n.max ? : 1 << nm->n.width;
    _nm.min_m = 1;
    _nm.max_m = nm->m.max ? : 1 << nm->m.width;

    if (ccu_sdm_helper_has_rate(&nm->common, &nm->sdm, rate))
    {
        ccu_sdm_helper_enable(&nm->common, &nm->sdm, rate);

        /* Sigma delta modulation requires specific N and M factors */
        ccu_sdm_helper_get_factors(&nm->common, &nm->sdm, rate,
                                   &_nm.m, &_nm.n);
    }
    else
    {
        ccu_sdm_helper_disable(&nm->common, &nm->sdm);
        ccu_nm_find_best(parent_rate, rate, &_nm);
    }

    __cspr = hal_spin_lock_irqsave(&nm->common.lock);

    reg = readl(nm->common.base + nm->common.reg);
    reg &= ~GENMASK(nm->n.width + nm->n.shift - 1, nm->n.shift);
    reg &= ~GENMASK(nm->m.width + nm->m.shift - 1, nm->m.shift);

    reg |= (_nm.n - nm->n.offset) << nm->n.shift;
    reg |= (_nm.m - nm->m.offset) << nm->m.shift;
    writel(reg, nm->common.base + nm->common.reg);

    hal_spin_unlock_irqrestore(&nm->common.lock, __cspr);

    ccu_helper_wait_for_lock(&nm->common, nm->lock);

    return 0;
}

static int ccu_pll12_nm_set_rate(struct clk_hw *hw, unsigned long rate,
		unsigned long parent_rate)
{
	struct ccu_nm *nm = hw_to_ccu_nm(hw);
	u32 reg;

	if (rate != 1920000000)
		printf("Warning: dpll1&dpll2 rate fixed to 1920M, can not changge to rate:%ld\n", rate);

	switch (parent_rate)
	{
		case 24000000:
			reg = 0xE0000501;
			break;
		case 24576000:
			reg = 0xE40014e1;
			break;
		case 26000000:
			reg = 0xFB13d4d4;
			break;
		case 32000000:
			reg = 0xE00003c1;
			break;
		case 40000000:
			reg = 0xE0000301;
			break;
		default:
			return 0;
	}

	writel(reg, nm->common.base + nm->common.reg);
	ccu_helper_wait_for_lock(&nm->common, nm->lock);

	return 0;
}

const struct clk_ops ccu_nm_ops =
{
    .disable    = ccu_nm_disable,
    .enable     = ccu_nm_enable,
    .is_enabled = ccu_nm_is_enabled,

    .recalc_rate    = ccu_nm_recalc_rate,
    .round_rate = ccu_nm_round_rate,
    .set_rate   = ccu_nm_set_rate,
};

const struct clk_ops ccu_pll12_nm_ops =
{
    .disable    = ccu_nm_disable,
    .enable     = ccu_nm_enable,
    .is_enabled = ccu_nm_is_enabled,

    .recalc_rate    = ccu_pll12_nm_recalc_rate,
    .round_rate = ccu_nm_round_rate,
    .set_rate   = ccu_pll12_nm_set_rate,
};
