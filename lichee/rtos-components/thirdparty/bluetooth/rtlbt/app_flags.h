/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      app_flags.h
   * @brief     This file is used to config app functions.
   * @author    jane
   * @date      2017-06-06
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */
#ifndef _APP_FLAGS_H_
#define _APP_FLAGS_H_

#include <stdint.h>
#include "bt_flags.h"

/** @defgroup  PERIPH_Config Peripheral App Configuration
    * @brief This file is used to config app functions.
    * @{
    */
/*============================================================================*
 *                              Constants
 *============================================================================*/

/** @brief  Config APP LE link number */
#define APP_MAX_LINKS  1
#define BT_GATT_SERVER_MAX_NUM  7
#define DEFAULT_ADVERTISING_INTERVAL_MIN 0x0040
#define DEFAULT_ADVERTISING_INTERVAL_MAX 0x0040

/** @} */ /* End of group PERIPH_Config */
#endif
