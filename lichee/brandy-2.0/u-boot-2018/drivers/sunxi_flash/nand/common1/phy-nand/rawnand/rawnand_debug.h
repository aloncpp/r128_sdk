/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __SPINAND_DEBUG_H__
#define __SPINAND_DEBUG_H__
#include <vsprintf.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include "controller/ndfc_base.h"
#include <sunxi_nand.h>

#define DBG
#ifdef DBG
#define RAWNAND_INFO(fmt, ...) \
	pr_info("[NI]" fmt, ##__VA_ARGS__)
/*
 *#define RAWNAND_DBG(fmt, ...) \
 *        pr_debug("[ND]" fmt, ##__VA_ARGS__)
 */

#define RAWNAND_DBG(fmt, ...) \
	pr_debug("[ND]" fmt, ##__VA_ARGS__)
#define RAWNAND_ERR(fmt, ...) \
	pr_err("[NE]" fmt, ##__VA_ARGS__)
#else
#define RAWNAND_INFO(fmt, ...)
/*
 *#define RAWNAND_DBG(fmt, ...) \
 *        pr_debug("[ND]" fmt, ##__VA_ARGS__)
 */

#define RAWNAND_DBG(fmt, ...)
#define RAWNAND_ERR(fmt, ...)
#endif

#define rawnand_print(fmt, arg...)                    \
	do {                                          \
		pr_err("[rawnand] " fmt "\n", ##arg); \
	} while (0)


void show_nctri(struct nand_controller_info *nctri);
void show_nci(struct nand_chip_info *nci);
void show_nsci(struct nand_super_chip_info *nsci);
void show_nsi(void);
void show_nssi(void);
void show_static_info(void);
void show_spare(int flag);
void nand_show_data(uchar *buf, u32 len);
void show_dict_page(unsigned int chip, unsigned int block, unsigned int page, u32 start, u32 len);
void show_logic_page_all_history(unsigned int logic_page);
int nand_super_page_test(unsigned int chip, unsigned int block, unsigned int page, unsigned char *mbuf);
int nand_phy_page_test(unsigned int chip, unsigned int block, unsigned int page, unsigned char *mbuf);
int nand_phy_block_test(unsigned int chip, unsigned int block, unsigned char *mbuf, u32 test_bad, u8 dat);
int nand_phy_block_erase_test(unsigned int chip, unsigned int block);
void nand_phy_erase_all(void);
void nand_phy_check_all(u8 dat);
int nand_phy_read_block_test(unsigned int chip, unsigned int start_block, unsigned int end_block);
int nand_update_end(void);
void nand_special_test(void);
extern void NAND_Print_Version(void);
#endif /*SPINAND_DEBUG_H*/
