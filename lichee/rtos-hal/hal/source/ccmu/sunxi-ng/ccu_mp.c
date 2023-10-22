// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2016 Maxime Ripard
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 */
#include "ccu.h"
#include "ccu_gate.h"
#include "ccu_mp.h"
#include <limits.h>

#define INDEX_MODE  1
#define NORMAL_MODE 2

static inline unsigned int ilog2(unsigned int v)
{
    unsigned int r;
    unsigned int shift;
    r = (v > 0xffff) << 4;
    v >>= r;
    shift = (v > 0xff) << 3;
    v >>= shift;
    r |= shift;
    shift = (v > 0xf) << 2;
    v >>= shift;
    r |= shift;
    shift = (v > 0x3) << 1;
    v >>= shift;
    r |= shift;
    r |= (v >> 1);
    return r;
}

static void ccu_mp_find_best(int mode, unsigned long parent, unsigned long rate,
                             unsigned int max_m, unsigned int max_p,
                             unsigned int *m, unsigned int *p)
{
    unsigned long best_rate = 0;
    unsigned int best_m = 0, best_p = 0;
    unsigned int _m, _p;

    if (mode == INDEX_MODE) {
        for (_p = 1; _p <= max_p; _p <<= 1) {
            for (_m = 1; _m <= max_m; _m++) {
                unsigned long tmp_rate = parent / _p / _m;

                if (tmp_rate > rate)
                    continue;

                if ((rate - tmp_rate) < (rate - best_rate)) {
                    best_rate = tmp_rate;
                    best_m = _m;
                    best_p = _p;
                }
            }
        }
    } else if (mode == NORMAL_MODE) {
        for (_p = 1; _p <= max_p; _p++) {
            for (_m = 1; _m <= max_m; _m++) {
                unsigned long tmp_rate = parent / _p / _m;

                if (tmp_rate > rate)
                    continue;

                if ((rate - tmp_rate) < (rate - best_rate)) {
                    best_rate = tmp_rate;
                    best_m = _m;
                    best_p = _p;
                }
            }
        }
    } else {
        pr_err("No such a mode, ccu find best fail!\n");
        best_m = 1;
        best_p = 1;
    }

    *m = best_m;
    *p = best_p;
}

static unsigned long ccu_mp_find_best_with_parent_adj(struct clk_hw *hw,
        unsigned long *parent,
        unsigned long rate,
        unsigned int max_m,
        unsigned int max_p)
{
    unsigned long parent_rate_saved;
    unsigned long parent_rate, now;
    unsigned long best_rate = 0;
    unsigned int _m, _p, div;
    unsigned long maxdiv;

    parent_rate_saved = *parent;

    /*
     * The maximum divider we can use without overflowing
     * unsigned long in rate * m * p below
     */
    maxdiv = max_m * max_p;
    maxdiv = min(ULONG_MAX / rate, maxdiv);

    for (_p = 1; _p <= max_p; _p <<= 1)
    {
        for (_m = 1; _m <= max_m; _m++)
        {
            div = _m * _p;

            if (div > maxdiv)
            {
                break;
            }

            if (rate * div == parent_rate_saved)
            {
                /*
                 * It's the most ideal case if the requested
                 * rate can be divided from parent clock without
                 * needing to change parent rate, so return the
                 * divider immediately.
                 */
                *parent = parent_rate_saved;
                return rate;
            }

            parent_rate = clk_hw_round_rate(hw, rate * div);
            now = parent_rate / div;

            if (now <= rate && now > best_rate)
            {
                best_rate = now;
                *parent = parent_rate;

                if (now == rate)
                {
                    return rate;
                }
            }
        }
    }

    return best_rate;
}

static unsigned long ccu_mp_round_rate(struct ccu_mux_internal *mux,
                                       struct clk_hw *hw,
                                       unsigned long *parent_rate,
                                       unsigned long rate,
                                       void *data)
{
    struct ccu_mp *cmp = data;
    unsigned int max_m, max_p;
    unsigned int m, p;

    if (cmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate *= cmp->fixed_post_div;
    }

    max_m = cmp->m.max ? : 1 << cmp->m.width;
    if (cmp->common.features & CCU_FEATURE_MP_NO_INDEX_MODE)
        max_p = cmp->p.max ?: 1 << cmp->p.width;
    else
        max_p = cmp->p.max ?: 1 << ((1 << cmp->p.width) - 1);

    if (!(clk_hw_get_flags(hw) & CLK_SET_RATE_PARENT))
    {
        if (cmp->common.features & CCU_FEATURE_MP_NO_INDEX_MODE)
            ccu_mp_find_best(NORMAL_MODE, *parent_rate, rate, max_m, max_p, &m, &p);
        else
            ccu_mp_find_best(INDEX_MODE, *parent_rate, rate, max_m, max_p, &m, &p);
        rate = *parent_rate / p / m;
    }
    else
    {
        rate = ccu_mp_find_best_with_parent_adj(hw, parent_rate, rate,
                                                max_m, max_p);
    }

    if (cmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate /= cmp->fixed_post_div;
    }

    return rate;
}

static void ccu_mp_disable(struct clk_hw *hw)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);

    return ccu_gate_helper_disable(&cmp->common, cmp->enable);
}

static int ccu_mp_enable(struct clk_hw *hw)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);

    return ccu_gate_helper_enable(&cmp->common, cmp->enable);
}

static int ccu_mp_is_enabled(struct clk_hw *hw)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);

    return ccu_gate_helper_is_enabled(&cmp->common, cmp->enable);
}

static unsigned long ccu_mp_recalc_rate(struct clk_hw *hw,
                                        unsigned long parent_rate)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);
    unsigned long rate;
    unsigned int m, p;
    u32 reg;

    /* Adjust parent_rate according to pre-dividers */
    parent_rate = ccu_mux_helper_apply_prediv(&cmp->common, &cmp->mux, -1,
                  parent_rate);

    reg = readl(cmp->common.base + cmp->common.reg);

    m = reg >> cmp->m.shift;
    m &= (1 << cmp->m.width) - 1;
    m += cmp->m.offset;
    if (!m)
    {
        m++;
    }

    p = reg >> cmp->p.shift;
    p &= (1 << cmp->p.width) - 1;

    rate = (parent_rate >> p) / m;
    if (cmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate /= cmp->fixed_post_div;
    }

    return rate;
}

static int ccu_mp_determine_rate(struct clk_hw *hw,
                                 struct clk_rate_request *req)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);

    return ccu_mux_helper_determine_rate(&cmp->common, &cmp->mux,
                                         req, ccu_mp_round_rate, cmp);
}

static int ccu_mp_set_rate(struct clk_hw *hw, unsigned long rate,
                           unsigned long parent_rate)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);
    unsigned int max_m, max_p;
    unsigned int m, p;
    u32 reg;
    u32 __cspr;

    /* Adjust parent_rate according to pre-dividers */
    parent_rate = ccu_mux_helper_apply_prediv(&cmp->common, &cmp->mux, -1,
                  parent_rate);

    max_m = cmp->m.max ? : 1 << cmp->m.width;
    if (cmp->common.features & CCU_FEATURE_MP_NO_INDEX_MODE)
        max_p = cmp->p.max ?: 1 << cmp->p.width;
    else
        max_p = cmp->p.max ?: 1 << ((1 << cmp->p.width) - 1);

    /* Adjust target rate according to post-dividers */
    if (cmp->common.features & CCU_FEATURE_FIXED_POSTDIV)
    {
        rate = rate * cmp->fixed_post_div;
    }

    if (cmp->common.features & CCU_FEATURE_MP_NO_INDEX_MODE)
        ccu_mp_find_best(NORMAL_MODE, parent_rate, rate, max_m, max_p, &m, &p);
    else
        ccu_mp_find_best(INDEX_MODE, parent_rate, rate, max_m, max_p, &m, &p);
    __cspr = hal_spin_lock_irqsave(&cmp->common.lock);

    reg = readl(cmp->common.base + cmp->common.reg);
    reg &= ~GENMASK(cmp->m.width + cmp->m.shift - 1, cmp->m.shift);
    reg &= ~GENMASK(cmp->p.width + cmp->p.shift - 1, cmp->p.shift);
    reg |= (m - cmp->m.offset) << cmp->m.shift;

    if (cmp->common.features & CCU_FEATURE_MP_NO_INDEX_MODE)
        reg |= (p - cmp->p.offset) << cmp->p.shift;
    else
        reg |= ilog2(p) << cmp->p.shift;

    writel(reg, cmp->common.base + cmp->common.reg);

    hal_spin_unlock_irqrestore(&cmp->common.lock, __cspr);

    return 0;
}

static u8 ccu_mp_get_parent(struct clk_hw *hw)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);

    return ccu_mux_helper_get_parent(&cmp->common, &cmp->mux);
}

static int ccu_mp_set_parent(struct clk_hw *hw, u8 index)
{
    struct ccu_mp *cmp = hw_to_ccu_mp(hw);

    return ccu_mux_helper_set_parent(&cmp->common, &cmp->mux, index);
}

const struct clk_ops ccu_mp_ops =
{
    .disable    = ccu_mp_disable,
    .enable     = ccu_mp_enable,
    .is_enabled = ccu_mp_is_enabled,

    .get_parent = ccu_mp_get_parent,
    .set_parent = ccu_mp_set_parent,

    .determine_rate = ccu_mp_determine_rate,
    .recalc_rate    = ccu_mp_recalc_rate,
    .set_rate   = ccu_mp_set_rate,
};

/*
 * Support for MMC timing mode switching
 *
 * The MMC clocks on some SoCs support switching between old and
 * new timing modes. A platform specific API is provided to query
 * and set the timing mode on supported SoCs.
 *
 * In addition, a special class of ccu_mp_ops is provided, which
 * takes in to account the timing mode switch. When the new timing
 * mode is active, the clock output rate is halved. This new class
 * is a wrapper around the generic ccu_mp_ops. When clock rates
 * are passed through to ccu_mp_ops callbacks, they are doubled
 * if the new timing mode bit is set, to account for the post
 * divider. Conversely, when clock rates are passed back, they
 * are halved if the mode bit is set.
 */

static unsigned long ccu_mp_mmc_recalc_rate(struct clk_hw *hw,
        unsigned long parent_rate)
{
    unsigned long rate = ccu_mp_recalc_rate(hw, parent_rate);
    struct ccu_common *cm = hw_to_ccu_common(hw);
    u32 val = readl(cm->base + cm->reg);

    if (val & CCU_MMC_NEW_TIMING_MODE)
    {
        return rate / 2;
    }
    return rate;
}

static int ccu_mp_mmc_determine_rate(struct clk_hw *hw,
                                     struct clk_rate_request *req)
{
    struct ccu_common *cm = hw_to_ccu_common(hw);
    u32 val = readl(cm->base + cm->reg);
    int ret;

    /* adjust the requested clock rate */
    if (val & CCU_MMC_NEW_TIMING_MODE)
    {
        req->rate *= 2;
        req->min_rate *= 2;
        req->max_rate *= 2;
    }

    ret = ccu_mp_determine_rate(hw, req);

    /* re-adjust the requested clock rate back */
    if (val & CCU_MMC_NEW_TIMING_MODE)
    {
        req->rate /= 2;
        req->min_rate /= 2;
        req->max_rate /= 2;
    }

    return ret;
}

static int ccu_mp_mmc_set_rate(struct clk_hw *hw, unsigned long rate,
                               unsigned long parent_rate)
{
    struct ccu_common *cm = hw_to_ccu_common(hw);
    u32 val = readl(cm->base + cm->reg);

    if (val & CCU_MMC_NEW_TIMING_MODE)
    {
        rate *= 2;
    }

    return ccu_mp_set_rate(hw, rate, parent_rate);
}

const struct clk_ops ccu_mp_mmc_ops =
{
    .disable    = ccu_mp_disable,
    .enable     = ccu_mp_enable,
    .is_enabled = ccu_mp_is_enabled,

    .get_parent = ccu_mp_get_parent,
    .set_parent = ccu_mp_set_parent,

    .determine_rate = ccu_mp_mmc_determine_rate,
    .recalc_rate    = ccu_mp_mmc_recalc_rate,
    .set_rate   = ccu_mp_mmc_set_rate,
};
