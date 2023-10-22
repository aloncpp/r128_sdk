//SPDX-License-Identifier:	GPL-2.0+
/*
 ************************************************************************************************************************
 *                                                      eNand
 *                                     Nand flash driver module config define
 *
 *                             Copyright(C), 2006-2008, SoftWinners Microelectronic Co., Ltd.
 *											       All Rights Reserved
 *
 * File Name : nand_drv_cfg.h
 *
 * Author : Kevin.z
 *
 * Version : v0.1
 *
 * Date : 2008.03.19
 *
 * Description : This file define the module config for nand flash driver.
 *               if need support some module /
 *               if need support some operation type /
 *               config limit for some parameter. ex.
 *
 * Others : None at present.
 *
 *
 * History :
 *
 *  <Author>        <time>       <version>      <description>
 *
 * Kevin.z         2008.03.19      0.1          build the file
 *
 ************************************************************************************************************************
 */
#ifndef __NAND_DRV_CFG_H
#define __NAND_DRV_CFG_H

#include "../../osal/nand_osal.h"

//#define  NAND_VERSION                 0x0223
//#define  NAND_DRV_DATE                  0x20150312

#define SUPPORT_UPDATE_EXTERNAL_ACCESS_FREQ (1)

#define SUPPORT_CHANGE_ONFI_TIMING_MODE (1)

#define SUPPORT_SCAN_EDO_FOR_SDR_NAND (0)
#define GOOD_DDR_EDO_DELAY_CHAIN_TH (7)

#define MAX_SECTORS_PER_PAGE_FOR_TWO_PLANE (32)

#if 1
#define PHY_DBG(...) PRINT_DBG(__VA_ARGS__)
#else
#define PHY_DBG(...)
#endif

#if 1
#define PHY_ERR(...) PRINT(__VA_ARGS__)
#else
#define PHY_ERR(...)
#endif

#endif //ifndef __NAND_DRV_CFG_H
