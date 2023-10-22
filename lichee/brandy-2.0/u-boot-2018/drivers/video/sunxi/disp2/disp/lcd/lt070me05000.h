/*
 * drivers/video/sunxi/disp2/disp/lcd/lt070me05000.h
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef  __LT070ME05000_PANEL_H__
#define  __LT070ME05000_PANEL_H__

#include "panels.h"

extern __lcd_panel_t lt070me05000_panel;

extern s32 dsi_dcs_wr_0para(u32 sel,u8 cmd);
extern s32 dsi_dcs_wr_1para(u32 sel,u8 cmd,u8 para);
extern s32 dsi_dcs_wr_2para(u32 sel,u8 cmd,u8 para1,u8 para2);
extern s32 dsi_dcs_wr_3para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3);
extern s32 dsi_dcs_wr_4para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3,u8 para4);
extern s32 dsi_dcs_wr_5para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3,u8 para4,u8 para5);
extern s32 dsi_gen_wr_0para(u32 sel,u8 cmd);
extern s32 dsi_gen_wr_1para(u32 sel,u8 cmd,u8 para);
extern s32 dsi_gen_wr_2para(u32 sel,u8 cmd,u8 para1,u8 para2);
extern s32 dsi_gen_wr_3para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3);
extern s32 dsi_gen_wr_4para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3,u8 para4);
extern s32 dsi_gen_wr_5para(u32 sel,u8 cmd,u8 para1,u8 para2,u8 para3,u8 para4,u8 para5);

#endif
