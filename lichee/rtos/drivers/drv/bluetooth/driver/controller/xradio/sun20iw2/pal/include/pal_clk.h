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
#ifndef PAL_CLK_H
#define PAL_CLK_H

void pal_clk_enable_ble_32m(void);
void pal_clk_disable_ble_32m(void);
void pal_clk_enable_ble_48m(void);
void pal_clk_disable_ble_48m(void);
void pal_clk_set_ble_sel_parent_losc_32k(void);
void pal_clk_set_ble_sel_parent_rccail_32k(void);
void pal_clk_enable_ble_div_32k(void);
void pal_clk_disable_ble_div_32k(void);
void pal_clk_enable_ble_auto_sw_32k(void);
void pal_clk_disable_ble_auto_sw_32k(void);

#endif /*PAL_CLK_H*/


