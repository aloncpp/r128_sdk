/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      reset driver .
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

#include "pal_reset.h"
#include "hal_clk.h"
#include "hal_reset.h"

/************************************************************************************************
* @Function: pal_reset_force_btcore
* @Description: force BT core, signal hrestn is 0
*               set 0, assert as mean as Force
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/
void pal_reset_force_btcore(void)
{
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_reset_id_t  reset_id = RST_BT_CORE;
	struct reset_control *reset;

	reset = hal_reset_control_get(reset_type, reset_id);
	if (hal_reset_control_assert(reset)) {
		printf("reset Force failed!");
	}
	hal_reset_control_put(reset);
}

/************************************************************************************************
* @Function: pal_reset_release_btcore
* @Description: release BT core, signal hrestn is 1
*               set 1,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/
void pal_reset_release_btcore(void)
{
	hal_reset_type_t reset_type = HAL_SUNXI_RESET;
	hal_reset_id_t reset_id = RST_BT_CORE;
	struct reset_control *reset;

	reset = hal_reset_control_get(reset_type, reset_id);
	if (hal_reset_control_deassert(reset)) {
		printf("reset Release failed!");
	}
	hal_reset_control_put(reset);
}
/************************************************************************************************
* @Function: pal_reset_force_bt_rtc
* @Description: force the global reset of the BLE RTC
*               set 0,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/

void pal_reset_force_bt_rtc(void)
{
	struct reset_control *reset;

	reset = hal_reset_control_get(HAL_SUNXI_AON_RESET, RST_BLE_RTC);
	hal_reset_control_assert(reset);
	hal_reset_control_put(reset);
}
/************************************************************************************************
* @Function: pal_reset_release_bt_rtc
* @Description: release the global reset of the BLE RTC
*               set 1,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/

void pal_reset_release_bt_rtc(void)
{
	struct reset_control *reset;

	reset = hal_reset_control_get(HAL_SUNXI_AON_RESET, RST_BLE_RTC);
	hal_reset_control_deassert(reset);
	hal_reset_control_put(reset);
}
/************************************************************************************************
* @Function: pal_reset_force_rfas
* @Description: force the global reset of the RFAS
*               set 0,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/
void pal_reset_force_rfas(void)
{
	struct reset_control *reset;

	reset = hal_reset_control_get(HAL_SUNXI_AON_RESET, RST_RFAS);
	hal_reset_control_assert(reset);
	hal_reset_control_put(reset);
}

/************************************************************************************************
* @Function: pal_reset_force_rfas
* @Description: release the global reset of the RFAS
*               set 1,deassert as mean as release
* @Parameters:
* # void
* @Return values:
* # void
*************************************************************************************************/
void pal_reset_release_rfas(void)
{
	struct reset_control *reset;

	reset = hal_reset_control_get(HAL_SUNXI_AON_RESET, RST_RFAS);
	hal_reset_control_deassert(reset);
	hal_reset_control_put(reset);
}

