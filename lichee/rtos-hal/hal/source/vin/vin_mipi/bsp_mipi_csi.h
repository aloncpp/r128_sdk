/*
 * sunxi mipi bsp interface
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  raymonxiu <raymonxiu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MIPI__CSI__H__
#define __MIPI__CSI__H__

#include "protocol.h"
#include "../utility/vin_common.h"

#define MAX_MIPI  1
#define MAX_MIPI_CH 4

struct mipi_para {
	unsigned int auto_check_bps;
	unsigned int bps;
	unsigned int dphy_freq;
	unsigned int lane_num;
	unsigned int total_rx_ch;
};

struct mipi_fmt_cfg {
	enum v4l2_field field[MAX_MIPI_CH];
	enum pkt_fmt packet_fmt[MAX_MIPI_CH];
	unsigned int fmt_type;
	unsigned int vc[MAX_MIPI_CH];
};

extern void bsp_mipi_csi_set_version(unsigned int sel, unsigned int ver);
extern int bsp_mipi_csi_set_base_addr(unsigned int sel,
				      unsigned long addr_base);
extern int bsp_mipi_dphy_set_base_addr(unsigned int sel,
				       unsigned long addr_base);
extern void mipi_dphy_cfg_1data(unsigned int sel,
				 unsigned int code, unsigned int data);
extern void bsp_mipi_csi_dphy_init(unsigned int sel);
extern void bsp_mipi_csi_dphy_exit(unsigned int sel);
extern void bsp_mipi_csi_dphy_enable(unsigned int sel, unsigned int flags);
extern void bsp_mipi_csi_dphy_disable(unsigned int sel, unsigned int flags);
extern void bsp_mipi_csi_protocol_enable(unsigned int sel);
extern void bsp_mipi_csi_protocol_disable(unsigned int sel);
extern void bsp_mipi_csi_set_para(unsigned int sel, struct mipi_para *para);
extern void bsp_mipi_csi_set_fmt(unsigned int sel, unsigned int total_rx_ch,
				 struct mipi_fmt_cfg *fmt);
void bsp_mipi_csi_set_dol(unsigned int sel, unsigned int mode, unsigned int ch);
#endif /*__MIPI__CSI__H__*/
