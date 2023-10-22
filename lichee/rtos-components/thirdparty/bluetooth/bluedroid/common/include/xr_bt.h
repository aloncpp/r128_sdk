// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __XR_BT_H__
#define __XR_BT_H__

#include <stdint.h>
#include <stdbool.h>
#include "xr_err.h"
#include "xr_task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bluetooth mode for controller enable/disable
 */
typedef enum {
    XR_BT_MODE_IDLE       = 0x00,   /*!< Bluetooth is not running */
    XR_BT_MODE_BLE        = 0x01,   /*!< Run BLE mode */
    XR_BT_MODE_CLASSIC_BT = 0x02,   /*!< Run Classic BT mode */
    XR_BT_MODE_BTDM       = 0x03,   /*!< Run dual mode */
} xr_bt_mode_t;

/**
 * @brief BLE sleep clock accuracy(SCA), values for ble_sca field in xr_bt_controller_config_t,
 *        currently only XR_BLE_SCA_500PPM and XR_BLE_SCA_250PPM are supported
 */

#ifdef CONFIG_BT_ENABLED
/* While scanning, if the free memory value in controller is less than SCAN_SEND_ADV_RESERVED_SIZE,
the adv packet will be discarded until the memory is restored. */
#define SCAN_SEND_ADV_RESERVED_SIZE        1000
/* enable controller log debug when adv lost */
#define CONTROLLER_ADV_LOST_DEBUG_BIT      (0<<0)

#ifdef CONFIG_BTDM_CTRL_HCI_UART_NO
#define BT_HCI_UART_NO_DEFAULT                      CONFIG_BTDM_CTRL_HCI_UART_NO
#else
#define BT_HCI_UART_NO_DEFAULT                      1
#endif /* BT_HCI_UART_NO_DEFAULT */

#ifdef CONFIG_BTDM_CTRL_HCI_UART_BAUDRATE
#define BT_HCI_UART_BAUDRATE_DEFAULT                CONFIG_BTDM_CTRL_HCI_UART_BAUDRATE
#else
#define BT_HCI_UART_BAUDRATE_DEFAULT                921600
#endif /* BT_HCI_UART_BAUDRATE_DEFAULT */

#ifdef CONFIG_BTDM_SCAN_DUPL_TYPE
#define SCAN_DUPLICATE_TYPE_VALUE  CONFIG_BTDM_SCAN_DUPL_TYPE
#else
#define SCAN_DUPLICATE_TYPE_VALUE  0
#endif

/* normal adv cache size */
#ifdef CONFIG_BTDM_SCAN_DUPL_CACHE_SIZE
#define NORMAL_SCAN_DUPLICATE_CACHE_SIZE            CONFIG_BTDM_SCAN_DUPL_CACHE_SIZE
#else
#define NORMAL_SCAN_DUPLICATE_CACHE_SIZE            20
#endif

#ifndef CONFIG_BTDM_BLE_MESH_SCAN_DUPL_EN
#define CONFIG_BTDM_BLE_MESH_SCAN_DUPL_EN FALSE
#endif

#define SCAN_DUPLICATE_MODE_NORMAL_ADV_ONLY         0
#define SCAN_DUPLICATE_MODE_NORMAL_ADV_MESH_ADV     1

#define BTDM_CONTROLLER_BLE_MAX_CONN_LIMIT          9   //Maximum BLE connection limitation
#define BTDM_CONTROLLER_BR_EDR_MAX_ACL_CONN_LIMIT   7   //Maximum ACL connection limitation
#define BTDM_CONTROLLER_BR_EDR_MAX_SYNC_CONN_LIMIT  3   //Maximum SCO/eSCO connection limitation

#define BTDM_CONTROLLER_SCO_DATA_PATH_HCI           0   // SCO data is routed to HCI
#define BTDM_CONTROLLER_SCO_DATA_PATH_PCM           1   // SCO data path is PCM


#else
#endif

/**
 * @brief Controller config options, depend on config mask.
 *        Config mask indicate which functions enabled, this means
 *        some options or parameters of some functions enabled by config mask.
 */
typedef struct {
    /*
     * Following parameters can be configured runtime, when call xr_bt_controller_init()
     */
    uint16_t controller_task_stack_size;    /*!< Bluetooth controller task stack size */
    uint8_t controller_task_prio;           /*!< Bluetooth controller task priority */
    uint8_t hci_uart_no;                    /*!< If use UART1/2 as HCI IO interface, indicate UART number */
    uint32_t hci_uart_baudrate;             /*!< If use UART1/2 as HCI IO interface, indicate UART baudrate */
    uint8_t scan_duplicate_mode;            /*!< scan duplicate mode */
    uint8_t scan_duplicate_type;            /*!< scan duplicate type */
    uint16_t normal_adv_size;               /*!< Normal adv size for scan duplicate */
    uint16_t mesh_adv_size;                 /*!< Mesh adv size for scan duplicate */
    uint16_t send_adv_reserved_size;        /*!< Controller minimum memory value */
    uint32_t  controller_debug_flag;        /*!< Controller debug log flag */
    uint8_t mode;                           /*!< Controller mode: BR/EDR, BLE or Dual Mode */
    uint8_t ble_max_conn;                   /*!< BLE maximum connection numbers */
    uint8_t bt_max_acl_conn;                /*!< BR/EDR maximum ACL connection numbers */
    uint8_t bt_sco_datapath;                /*!< SCO data path, i.e. HCI or PCM module */
    bool auto_latency;                      /*!< BLE auto latency, used to enhance classic BT performance */
    bool bt_legacy_auth_vs_evt;             /*!< BR/EDR Legacy auth complete event required to  protect from BIAS attack */
    /*
     * Following parameters can not be configured runtime when call xr_bt_controller_init()
     * It will be overwrite with a constant value which in menuconfig or from a macro.
     * So, do not modify the value when xr_bt_controller_init()
     */
    uint8_t bt_max_sync_conn;               /*!< BR/EDR maximum ACL connection numbers. Effective in menuconfig */
    uint8_t ble_sca;                        /*!< BLE low power crystal accuracy index */
    uint8_t pcm_role;                       /*!< PCM role (master & slave)*/
    uint8_t pcm_polar;                      /*!< PCM polar trig (falling clk edge & rising clk edge) */
    uint32_t magic;                         /*!< Magic number */
} xr_bt_controller_config_t;

/**
 * @brief Bluetooth audio data transport path
 */
typedef enum {
    XR_SCO_DATA_PATH_HCI = 0,            /*!< data over HCI transport */
    XR_SCO_DATA_PATH_PCM = 1,            /*!< data over PCM interface */
} xr_sco_data_path_t;

#ifdef __cplusplus
}
#endif

#endif /* __XR_BT_H__ */
