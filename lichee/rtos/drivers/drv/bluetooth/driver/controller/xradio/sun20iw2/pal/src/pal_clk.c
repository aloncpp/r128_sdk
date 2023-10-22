/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      BT clk driver.
 *
 *  Copyright (c) 2018-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include "hal_clk.h"
#include "pal_clk.h"
#include "hal_reset.h"
#include "ccu-sun20iw2-aon.h"
#include "common_ccmu.h"
#include "platform_prcm.h"

void pal_clk_enable_ble_48m(void)
{
	hal_clk_t clk;
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_BLE_48M);
	hal_clock_enable(clk);
}

void pal_clk_enable_ble_32m(void)
{
	hal_clk_t clk;
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_BLE_32M);
	hal_clock_enable(clk);
}

void pal_clk_disable_ble_48m(void)
{
	hal_clk_t clk;
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_BLE_48M);
	hal_clock_disable(clk);
}

void pal_clk_disable_ble_32m(void)
{
	hal_clk_t clk;
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_BLE_32M);
	hal_clock_disable(clk);
}

void pal_clk_set_ble_sel_parent_losc_32k(void)
{
	hal_clk_t lf_sel_clk, ble_clk_32K_sel;

	lf_sel_clk = hal_clock_get(HAL_SUNXI_R_CCU, CLK_LF_SEL);
	ble_clk_32K_sel = hal_clock_get(HAL_SUNXI_R_CCU, CLK_BLE_SEL);
	hal_clk_set_parent(ble_clk_32K_sel, lf_sel_clk);
}

void pal_clk_set_ble_sel_parent_rccail_32k(void)
{
	hal_clk_t rccail32k, ble_clk_32K_sel;

	rccail32k = hal_clock_get(HAL_SUNXI_R_CCU, CLK_RCCAL32K);
	ble_clk_32K_sel = hal_clock_get(HAL_SUNXI_R_CCU, CLK_BLE_SEL);
	hal_clk_set_parent(ble_clk_32K_sel, rccail32k);
}

void pal_clk_enable_ble_div_32k(void)
{
	/* set the source clk of div_clk */
	hal_clk_t div_clk, div_clk_parent;
	uint32_t div;
	div_clk = hal_clock_get(HAL_SUNXI_R_CCU, CLK_DIV);
	div_clk_parent = hal_clock_get(HAL_SUNXI_R_CCU, RC_HF_EN);
	hal_clk_set_parent(div_clk, div_clk_parent);

	/* set div_clk to 32k */
	div = HAL_GetHFClock() / (32 * 1000) / 2 - 1;

	HAL_MODIFY_REG(PRCM->BLE_CLK32K_SWITCH0, PRCM_BLE_CLK32K_DIV_MASK,
	               PRCM_BLE_CLK32K_DIV_VALUE(div));

	/* enable div clk */
	hal_clock_enable(div_clk);
}

void pal_clk_disable_ble_div_32k(void)
{
	/* disable div clk */
	hal_clk_t div_clk;
	div_clk = hal_clock_get(HAL_SUNXI_R_CCU, CLK_DIV);
	hal_clock_enable(div_clk);
}

void pal_clk_enable_ble_auto_sw_32k(void)
{
	hal_clk_t auto_sw_clk;
	auto_sw_clk = hal_clock_get(HAL_SUNXI_R_CCU, CLK_32K_AUTO_SWITCH);
	hal_clock_enable(auto_sw_clk);

}

void pal_clk_disable_ble_auto_sw_32k(void)
{
	hal_clk_t auto_sw_clk;
	auto_sw_clk = hal_clock_get(HAL_SUNXI_R_CCU, CLK_32K_AUTO_SWITCH);
	hal_clock_disable(auto_sw_clk);
}
