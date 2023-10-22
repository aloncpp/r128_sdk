/*
********************************************************************************
* @file    config.h
* @brief   This file contain all the MACRO that can control fuctional switch and DEBUG Logs.
******************************************************************************
*/
#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <limits.h>

#define CONFIG_POLL                      1
#define CONFIG_ASSERT                    1

//#define configUSE_MALLOC_FAILED_HOOK
//#define CONFIG_BT_RECV_IS_RX_THREAD 1
#define CONFIG_BT_HCI_HOST               1
//#define CONFIG_BT_SETTINGS               0
#define CONFIG_SYS_CLOCK_EXISTS

#define CONFIG_SYS_LOG

#define CONFIG_SYS_LOG_SHOW_TAGS

/*Subdebug Macro*/
#define CONFIG_SYS_LOG_NET_BUF_LEVEL   SYS_LOG_LEVEL_DEBUG//SYS_LOG_LEVEL_WARNING
#define CONFIG_SYS_LOG_DEFAULT_LEVEL   SYS_LOG_LEVEL_DEBUG//SYS_LOG_LEVEL_WARNING

/**** Bug Fix ****/
#define BT_MESH_FIXED_TRANSACTION_ID_CHECK     0

//#define CONFIG_BLEHOST_Z_ITERABLE_SECTION 1


#if 0
#define CONFIG_HCI_DEVICE "hci0"
#define CONFIG_SOC "Ubuntu_X86"

#define CONFIG_SYSTEM_WORKQUEUE_STACK_SIZE 1024
#define CONFIG_SYSTEM_WORKQUEUE_PRIORITY 3

#define CONFIG_BT_PERIPHERAL             1
#define CONFIG_BT_CENTRAL                1
#define CONFIG_BT_MESH                   1

#if CONFIG_BT_CENTRAL
#define CONFIG_BT_GATT_CLIENT            1
#else
#define CONFIG_BT_GATT_CLIENT            0
#endif

/* FUNCTIONAL MACRO */
//#define CONFIG_BT_HCI_ACL_FLOW_CONTROL   1  /* controller not supported right now */
#define CONFIG_BT_ECC                    1
//#define CONFIG_BT_BREDR                  0
#define CONFIG_BT_A2DP                   0
#define CONFIG_BT_AVDTP                  0
#define CONFIG_BT_RFCOMM                 0
#define CONFIG_BT_PRIVACY                1
#define CONFIG_BT_RPA                    1
#define CONFIG_BT_OBSERVER               1
#define CONFIG_BT_BROADCASTER            1
#define CONFIG_BT_CONN                   1
#define CONFIG_BT_HOST_CRYPTO            1
//#define CONFIG_BT_WHITELIST            1
#define CONFIG_BT_SCAN_WITH_IDENTITY     1
#define CONFIG_BT_AUTO_PHY_UPDATE        0
#define CONFIG_BT_SMP                    1
//#define CONFIG_BT_SMP_SC_PAIR_ONLY            0
#define CONFIG_BT_SIGNING
#define CONFIG_BT_GATT_SERVICE_CHANGED   1
#if (CONFIG_BT_GATT_SERVICE_CHANGED)
#define CONFIG_BT_GATT_DYNAMIC_DB        1
#define CONFIG_BT_GATT_CACHING			 1
#endif
#define CONFIG_BT_MESH_PROVISIONER 1
#define CONFIG_BT_SMP_FORCE_BREDR        0
#define CONFIG_BT_TINYCRYPT_ECC          1
#define CONFIG_BT_ATT_ENFORCE_FLOW       0
#define CONFIG_BT_FIXED_PASSKEY          0
#define CONFIG_BT_MESH_PROV              1
#define CONFIG_BT_MESH_PROXY             1
#define CONFIG_BT_MESH_PB_ADV            1
#define CONFIG_BT_MESH_PB_GATT           1
#define CONFIG_BT_MESH_LOW_POWER         1
#define CONFIG_BT_MESH_FRIEND            1
#define CONFIG_BT_MESH_GATT_PROXY        1
//#define CONFIG_BT_MESH_DEBUG_USE_ID_ADDR 1
#define CONFIG_BT_MESH_RELAY             1
#define CONFIG_BT_MESH_LPN_ESTABLISHMENT 1
#define CONFIG_BT_MESH_LPN_AUTO          0
#define CONFIG_SETTINGS                  0
//#define CONFIG_BT_MESH_CDB               1
#define CONFIG_BT_MESH_APP_KEY_COUNT     3
#define CONFIG_BT_MESH_SUBNET_COUNT      3

#define CONFIG_BT_DEVICE_APPEARANCE          0
#define CONFIG_BT_DEVICE_NAME_GATT_WRITABLE  1
#define CONFIG_BT_DEVICE_NAME_DYNAMIC        1
#define CONFIG_BT_DEVICE_NAME_MAX            32

#ifdef CONFIG_BT_BREDR
#define CONFIG_BT_L2CAP_DYNAMIC_CHANNEL  1
#endif

/* DEBUG MACRO */
#define CONFIG_BT_DEBUG_LOG				1
//#define BT_DBG_ENABLED           1
//#define CONFIG_BT_DEBUG_MONITOR

/*Subdebug Macro*/
//#define CONFIG_NET_BUF_LOG
//#define CONFIG_NET_BUF_SIMPLE_LOG
#define CONFIG_BT_DEBUG_SETTINGS       0
#define CONFIG_BT_DEBUG_RPA            0
#define CONFIG_BT_DEBUG_HCI_CORE       0
#define CONFIG_BT_DEBUG_CONN           0
#define CONFIG_BT_DEBUG_L2CAP          0
#define CONFIG_BT_DEBUG_ATT            0
#define CONFIG_BT_DEBUG_GATT           0
#define CONFIG_BT_DEBUG_SMP            0
#define CONFIG_BT_DEBUG_KEYS           0
//#define CONFIG_BT_USE_DEBUG_KEYS       1
#define CONFIG_BT_DEBUG_HCI_DRIVER     0
#define CONFIG_BT_MESH_DEBUG           0
#define CONFIG_BT_MESH_DEBUG_ACCESS    0
#define CONFIG_BT_MESH_DEBUG_ADV       0
#define CONFIG_BT_MESH_DEBUG_BEACON    0
#define CONFIG_BT_MESH_DEBUG_MODEL     0
#define CONFIG_BT_MESH_DEBUG_CRYPTO    0
#define CONFIG_BT_MESH_DEBUG_FRIEND    0
#define CONFIG_BT_MESH_DEBUG_LOW_POWER 0
#define CONFIG_BT_MESH_DEBUG_NET       0
#define CONFIG_BT_MESH_DEBUG_PROV      0
#define CONFIG_BT_MESH_DEBUG_PROXY     0
#define CONFIG_BT_MESH_DEBUG_TRANS     0

#define CONFIG_BT_TESTING              0
#define CONFIG_BT_MESH_IV_UPDATE_TEST  1

#define CONFIG_BT_MESH_SEQ_STORE_RATE 128
#define CONFIG_BT_MESH_RPL_STORE_TIMEOUT 600
#define CONFIG_BT_MESH_STORE_TIMEOUT 2

#ifdef CONFIG_BT_USERCHAN
#define CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE 24
#endif

#ifdef CONFIG_BT_H4
#define CONFIG_BT_UART_ON_DEV_NAME "/dev/ttyACM0"
#endif

#ifdef CONFIG_ASSERT
#define CONFIG_ASSERT_LEVEL 2
#endif

#ifndef CONFIG_BT_ID_MAX
#define CONFIG_BT_ID_MAX 1
#endif

#ifndef CONFIG_BT_MESH_LPN_AUTO_TIMEOUT
#define CONFIG_BT_MESH_LPN_AUTO_TIMEOUT 15
#endif

/*
Ranges:	[0x0001, 0xffff]
*/
#ifndef CONFIG_BT_PAGE_TIMEOUT
#define CONFIG_BT_PAGE_TIMEOUT 0x2000
#endif

#ifndef CONFIG_BT_CREATE_CONN_TIMEOUT
#define CONFIG_BT_CREATE_CONN_TIMEOUT 3
#endif

#ifndef CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT
#define CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT 5000
#endif

/*
range 4 65535 if BT && NET_BUF
range 0 65535 if NET_BUF
default 4 if NET_BUF
*/
#ifndef CONFIG_NET_BUF_USER_DATA_SIZE
#define CONFIG_NET_BUF_USER_DATA_SIZE 4
#endif

/*
range 65 2000 if BT_SMP && BT_CONN && BT_HCI_HOST && BT_HCI && BT
range 23 2000 if BT_CONN && BT_HCI_HOST && BT_HCI && BT
default 253 if BT_BREDR && BT_CONN && BT_HCI_HOST && BT_HCI && BT
default 65 if BT_SMP && BT_CONN && BT_HCI_HOST && BT_HCI && BT
default 23 if BT_CONN && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_L2CAP_TX_MTU
#define CONFIG_BT_L2CAP_TX_MTU 259
#endif

#ifndef CONFIG_BT_BACKGROUND_SCAN_INTERVAL
#define CONFIG_BT_BACKGROUND_SCAN_INTERVAL 2048
#endif

#ifndef CONFIG_BT_BACKGROUND_SCAN_WINDOW
#define CONFIG_BT_BACKGROUND_SCAN_WINDOW 18
#endif

#ifndef CONFIG_BT_ATT_TX_MAX
#define CONFIG_BT_ATT_TX_MAX 2
#endif

#ifndef CONFIG_BT_MESH_MODEL_KEY_COUNT
#define CONFIG_BT_MESH_MODEL_KEY_COUNT 1  /*[1,4096]*/
#endif

#ifndef CONFIG_BT_MESH_MODEL_GROUP_COUNT
#define CONFIG_BT_MESH_MODEL_GROUP_COUNT 1  /*[1,4096]*/
#endif

/*
range 2 32 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 3 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_TX_SEG_MAX
#define CONFIG_BT_MESH_TX_SEG_MAX 3
#endif

/*
range 1 4096 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_APP_KEY_COUNT
#define CONFIG_BT_MESH_APP_KEY_COUNT 1
#endif

/*
range 1 4096 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_SUBNET_COUNT
#define CONFIG_BT_MESH_SUBNET_COUNT 1
#endif

/*
range 2 65535 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 10 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_CRPL
#define CONFIG_BT_MESH_CRPL 10
#endif

/*
range 6 256 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 6 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_ADV_BUF_COUNT
#define CONFIG_BT_MESH_ADV_BUF_COUNT 6
#endif

#ifndef CONFIG_BT_MESH_MODEL_KEY_COUNT
#define CONFIG_BT_MESH_MODEL_KEY_COUNT 1
#endif

/*
range 0 4096 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LABEL_COUNT
#define CONFIG_BT_MESH_LABEL_COUNT 1
#endif

#ifdef CONFIG_BT_MESH_FRIEND
/*
range 0 1023 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 3 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE
#define CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE 3
#endif
#ifndef CONFIG_BT_MESH_FRIEND_SEG_RX
#define CONFIG_BT_MESH_FRIEND_SEG_RX 1 /*[1,1000]*/
#endif
/*
range 1 1000 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 2 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_FRIEND_LPN_COUNT
#define CONFIG_BT_MESH_FRIEND_LPN_COUNT 2
#endif
/*
range 1 255 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 255 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_FRIEND_RECV_WIN
#define CONFIG_BT_MESH_FRIEND_RECV_WIN 255
#endif
/*
range 2 65536 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 16 if BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_FRIEND && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_FRIEND_QUEUE_SIZE
#define CONFIG_BT_MESH_FRIEND_QUEUE_SIZE 16
#endif
#endif //CONFIG_BT_MESH_FRIEND

#ifdef CONFIG_BT_MESH_LOW_POWER
/*
range 10 244735 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 300 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_POLL_TIMEOUT
#define CONFIG_BT_MESH_LPN_POLL_TIMEOUT 300
#endif
/*
range 10 255 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 100 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_RECV_DELAY
#define CONFIG_BT_MESH_LPN_RECV_DELAY 100
#endif
#ifndef CONFIG_BT_MESH_LPN_GROUPS
#define CONFIG_BT_MESH_LPN_GROUPS 8 /*[0, 16384]*/
#endif
#ifndef CONFIG_BT_MESH_LPN_INIT_POLL_TIMEOUT
#define CONFIG_BT_MESH_LPN_INIT_POLL_TIMEOUT 10
#endif
/*
range 0 50 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 10 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_SCAN_LATENCY
#define CONFIG_BT_MESH_LPN_SCAN_LATENCY 10
#endif
/*
range 1 3600 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 8 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_RETRY_TIMEOUT
#define CONFIG_BT_MESH_LPN_RETRY_TIMEOUT 8
#endif
/*
range 1 7 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_MIN_QUEUE_SIZE
#define CONFIG_BT_MESH_LPN_MIN_QUEUE_SIZE 1
#endif
/*
range 0 3 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 0 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_RSSI_FACTOR
#define CONFIG_BT_MESH_LPN_RSSI_FACTOR 0
#endif
/*
range 0 3 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 0 if BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_LOW_POWER && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_LPN_RECV_WIN_FACTOR
#define CONFIG_BT_MESH_LPN_RECV_WIN_FACTOR 0
#endif
#endif //CONFIG_BT_MESH_LOW_POWER

/*
range 2 65535 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 10 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_MSG_CACHE_SIZE
#define CONFIG_BT_MESH_MSG_CACHE_SIZE 10
#endif

/*
range 2 96 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 4 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_IVU_DIVIDER
#define CONFIG_BT_MESH_IVU_DIVIDER 4
#endif

/*
range 1 32767 if BT_MESH_PROXY && BT_CONN && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH_PROXY && BT_CONN && BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 3 if BT_MESH_GATT_PROXY && BT_MESH_PROXY && BT_CONN && BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH_PROXY && BT_CONN && BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_PROXY_FILTER_SIZE
#define CONFIG_BT_MESH_PROXY_FILTER_SIZE 3
#endif

/*
range 1 BT_MESH_ADV_BUF_COUNT if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_TX_SEG_MSG_COUNT
#define CONFIG_BT_MESH_TX_SEG_MSG_COUNT 1
#endif

/*
range 1 255 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 1 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_RX_SEG_MSG_COUNT
#define CONFIG_BT_MESH_RX_SEG_MSG_COUNT 1
#endif

/*
range 24 384 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
default 72 if BT_MESH && BT_HCI_HOST && BT_HCI && BT
depends on BT_MESH && BT_HCI_HOST && BT_HCI && BT
*/
#ifndef CONFIG_BT_MESH_RX_SDU_MAX
#define CONFIG_BT_MESH_RX_SDU_MAX 72
#endif

#ifndef CONFIG_BT_MESH_CDB_NODE_COUNT
#define CONFIG_BT_MESH_CDB_NODE_COUNT 1
#endif

#ifndef CONFIG_BT_MESH_CDB_SUBNET_COUNT
#define CONFIG_BT_MESH_CDB_SUBNET_COUNT 1
#endif

#ifndef CONFIG_BT_MESH_CDB_APP_KEY_COUNT
#define CONFIG_BT_MESH_CDB_APP_KEY_COUNT 1
#endif

/*****   TASK PRIORITY   *****/

/**
 * CONFIG_BT_HCI_RX_PRIO: rx thread priority
 */
/*
#ifndef CONFIG_BT_HCI_RX_PRIO
#define CONFIG_BT_HCI_RX_PRIO 41
#endif*/

#ifndef CONFIG_BT_RX_PRIO
#define CONFIG_BT_RX_PRIO 4
#endif
/**
 * CONFIG_BT_HCI_TX_PRIO: tx thread priority
 */
#ifndef CONFIG_BT_HCI_TX_PRIO
#define CONFIG_BT_HCI_TX_PRIO 3
#endif
/**
 *  CONFIG_BT_WORK_QUEUE_PRIO:Work queue priority.
 */
#ifndef CONFIG_BT_WORK_QUEUE_PRIO
#define CONFIG_BT_WORK_QUEUE_PRIO 1
#endif

#ifndef CONFIG_BT_CTLR_RX_PRIO  //Currently no use
#define CONFIG_BT_CTLR_RX_PRIO 4
#endif

/*****   TASK STACK SIZE   *****/
/**
 * CONFIG_BT_HCI_RX_STACK_SIZE: rx thread stack size
 */
/*#ifndef CONFIG_BT_HCI_RX_STACK_SIZE
#define CONFIG_BT_HCI_RX_STACK_SIZE 512
#endif*/

#ifndef CONFIG_BT_HCI_ECC_STACK_SIZE
#define CONFIG_BT_HCI_ECC_STACK_SIZE 1024
#endif

#ifndef CONFIG_BT_RX_STACK_SIZE
#define CONFIG_BT_RX_STACK_SIZE 1024
#endif

#ifndef CONFIG_BT_CTLR_RX_PRIO_STACK_SIZE  //Currently no use
#define CONFIG_BT_CTLR_RX_PRIO_STACK_SIZE 156
#endif
/**
 * CONFIG_BT: Tx thread stack size
 */
#ifndef CONFIG_BT_HCI_TX_STACK_SIZE
#define CONFIG_BT_HCI_TX_STACK_SIZE 512
#endif
/**
 *  CONFIG_BT_WORK_QUEUE_STACK_SIZE:Work queue stack size.
 */
#ifndef CONFIG_BT_WORK_QUEUE_STACK_SIZE
#define CONFIG_BT_WORK_QUEUE_STACK_SIZE 512
#endif

/*****   BUFFER SIZE   *****/
/**
 * CONFIG_BT_HCI_CMD_COUNT: hci cmd buffer count,range 2 to 64
 */
#ifndef CONFIG_BT_HCI_CMD_COUNT
#define CONFIG_BT_HCI_CMD_COUNT 2
#endif

/**
 * CONFIG_BT_RX_BUF_COUNT: number of buffer for incoming ACL packages or HCI
 * events,range 2 to 255
 */
#ifndef CONFIG_BT_RX_BUF_COUNT
#define CONFIG_BT_RX_BUF_COUNT 10
#endif

/**
 * CONFIG_BT_RX_BUF_LEN: the max length for rx buffer
 * range 73 to 2000
 */
#ifndef CONFIG_BT_RX_BUF_LEN
#define CONFIG_BT_RX_BUF_LEN 259// 76
#endif

#ifdef CONFIG_BT_CONN

/**
 * CONFIG_BLUETOOTH_L2CAP_TX_BUF_COUNT: number of buffer for outgoing L2CAP
 * packages range 2 to 255
 */
#ifdef BLE_APP_RECONFIG_MESH_SRV
#undef CONFIG_BT_L2CAP_TX_BUF_COUNT
#define CONFIG_BT_L2CAP_TX_BUF_COUNT 3
#else
#define CONFIG_BT_L2CAP_TX_BUF_COUNT 10
#endif

/**
 * CONFIG_BT_L2CAP_TX_MTU: Max L2CAP MTU for L2CAP tx buffer
 * range 65 to 2000 if SMP enabled,otherwise range 23 to 2000
 */
/*#ifndef CONFIG_BT_L2CAP_TX_MTU
#ifdef CONFIG_BT_SMP
#define CONFIG_BT_L2CAP_TX_MTU 65
#else
#define CONFIG_BT_L2CAP_TX_MTU 23
#endif
#endif*/

#define CONFIG_BT_ACL_RX_COUNT 6
#define CONFIG_BT_L2CAP_RX_MTU 300

/**
 * CONFIG_BT_L2CAP_TX_USER_DATA_SIZE: the max length for L2CAP tx buffer user
 * data size range 4 to 65535
 */
#ifndef CONFIG_BT_L2CAP_TX_USER_DATA_SIZE
#define CONFIG_BT_L2CAP_TX_USER_DATA_SIZE 4
#endif

/**
 * CONFIG_BT_ATT_PREPARE_COUNT: Number of buffers available for ATT prepare
 * write, setting this to 0 disables GATT long/reliable writes. range 0 to 64
 */
#ifndef CONFIG_BT_ATT_PREPARE_COUNT
#define CONFIG_BT_ATT_PREPARE_COUNT 0
#endif

/**
 *  CONFIG_BLUETOOTH_SMP:Eable the Security Manager Protocol
 *  (SMP), making it possible to pair devices over LE
 */
#ifdef CONFIG_BT_SMP
/**
 *  CONFIG_BT_SIGNING:enables data signing which is used for transferring
 *  authenticated data in an unencrypted connection
 */
#ifdef CONFIG_BT_SIGNING
#undef CONFIG_BT_SIGNING
#define CONFIG_BT_SIGNING 1
#endif

/**
 *  CONFIG_BT_SMP_SC_ONLY:enables support for Secure Connection Only Mode. In
 * this mode device shall only use Security Mode 1 Level 4 with exception for
 * services that only require Security Mode 1 Level 1 (no security). Security
 * Mode 1 Level 4 stands for authenticated LE Secure Connections pairing with
 * encryption. Enabling this option disables legacy pairing
 */
/*#ifdef CONFIG_BT_SMP_SC_ONLY
#undef CONFIG_BT_SMP_SC_ONLY
#define CONFIG_BT_SMP_SC_ONLY 0
#endif*/

/**
 *  CONFIG_BT_USE_DEBUG_KEYS:This option places Security Manager in
 *  a Debug Mode. In this mode predefined
 *  Diffie-Hellman private/public key pair is used as described
 *  in Core Specification Vol. 3, Part H, 2.3.5.6.1. This option should
 *  only be enabled for debugging and should never be used in production.
 *  If this option is enabled anyone is able to decipher encrypted air
 *  traffic.
 */
#ifdef CONFIG_BT_USE_DEBUG_KEYS
#ifndef CONFIG_BT_TINYCRYPT_ECC
#error "CONFIG_BT_USE_DEBUG_KEYS depends on CONFIG_BT_TINYCRYPT_ECC"
#endif
#undef CONFIG_BT_USE_DEBUG_KEYS
#define CONFIG_BT_USE_DEBUG_KEYS 1
#endif

/**
 *  CONFIG_BT_L2CAP_DYNAMIC_CHANNEL:enables support for LE Connection
 *  oriented Channels,allowing the creation of dynamic L2CAP Channels
 */
#ifdef CONFIG_BT_L2CAP_DYNAMIC_CHANNEL
#undef CONFIG_BT_L2CAP_DYNAMIC_CHANNEL
#define CONFIG_BT_L2CAP_DYNAMIC_CHANNEL 1
#endif

#endif

/**
 *   CONFIG_BT_PRIVACY:Enable local Privacy Feature support. This makes it
 * possible to use Resolvable Private Addresses (RPAs).
 */

#ifndef CONFIG_BT_SMP
#error "CONFIG_BT_PRIVACY depends on CONFIG_BT_SMP"
#endif

/**
 * CONFIG_BT_RPA_TIMEOUT:Resolvable Private Address timeout
 * range 1 to 65535,seconds
 */
#ifndef CONFIG_BT_RPA_TIMEOUT
#define CONFIG_BT_RPA_TIMEOUT 900
#endif
#endif

/**
 *  CONFIG_BT_GATT_DYNAMIC_DB:enables GATT services to be added dynamically to
 * database
 */
#ifdef CONFIG_BT_GATT_DYNAMIC_DB
#undef CONFIG_BT_GATT_DYNAMIC_DB
#define CONFIG_BT_GATT_DYNAMIC_DB 1
#endif

/**
 *  CONFIG_BT_GATT_CLIENT:GATT client role support
 */
#ifdef CONFIG_BT_GATT_CLIENT
#undef CONFIG_BT_GATT_CLIENT
#define CONFIG_BT_GATT_CLIENT 1
#endif

/**
 *  CONFIG_BT_MAX_PAIRED:Maximum number of paired devices
 *  range 1 to 128
 */
#ifndef CONFIG_BT_MAX_PAIRED
#define CONFIG_BT_MAX_PAIRED 1
#endif


/**
 * If this option is set TinyCrypt library is used for emulating the
 * ECDH HCI commands and events needed by e.g. LE Secure Connections.
 * In builds including the BLE Host, if not set the controller crypto is
 * used for ECDH and if the controller doesn't support the required HCI
 * commands the LE Secure Connections support will be disabled.
 * In builds including the HCI Raw interface and the BLE Controller, this
 * option injects support for the 2 HCI commands required for LE Secure
 * Connections so that Hosts can make use of those
 */
/*#ifdef CONFIG_BT_TINYCRYPT_ECC
#undef CONFIG_BT_TINYCRYPT_ECC
#define CONFIG_BT_TINYCRYPT_ECC 0
#endif*/
/**
 *  CONFIG_BLUETOOTH_MAX_CONN:Maximum number of connections
 *  range 1 to 128
 */
#ifndef CONFIG_BT_MAX_CONN
#if CONFIG_BT_CENTRAL
#define CONFIG_BT_MAX_CONN 8
#else
#define CONFIG_BT_MAX_CONN 1
#endif
#endif

/**
 *  CONFIG_BT_DEVICE_NAME:Bluetooth device name. Name can be up
 *  to 248 bytes long (excluding NULL termination). Can be empty string
 */
#ifndef CONFIG_BT_DEVICE_NAME
#define CONFIG_BT_DEVICE_NAME "XR806"
#endif

/**
 *  CONFIG_BT_MAX_SCO_CONN:Maximum number of simultaneous SCO connections.
 */
#ifndef CONFIG_BT_MAX_SCO_CONN
#define CONFIG_BT_MAX_SCO_CONN 0
#endif

/**
 *  CONFIG_BT_HCI_RESERVE:Headroom that the driver needs for sending and
 * receiving buffers.
 */
#ifndef CONFIG_BT_HCI_RESERVE
#ifdef CONFIG_BLUETOOTH_H4
#define CONFIG_BT_HCI_RESERVE 0
#elif defined(CONFIG_BLUETOOTH_H5) || defined(CONFIG_BLUETOOTH_SPI)
#define CONFIG_BT_HCI_RESERVE 1
#else
#define CONFIG_BT_HCI_RESERVE 1
#endif
#endif

/**
 *  CONFIG_BLUETOOTH_DEBUG_LOG:Enable bluetooth debug log.
 */
#ifdef CONFIG_BT_DEBUG_LOG
#undef CONFIG_BT_DEBUG_LOG
#define CONFIG_BT_DEBUG_LOG 1
#undef CONFIG_BT_DEBUG
#define CONFIG_BT_DEBUG 1
#endif

/**
 *  CONFIG_BT_TEST:Enable bluetooth test.
 */
#ifdef CONFIG_BT_TEST
#undef CONFIG_BT_TEST
#define CONFIG_BT_TEST 1
#endif

/**
 *  CONFIG_BT_DEBUG_CORE:Enable bluetooth core debug log.
 */
#ifdef CONFIG_BT_DEBUG_CORE
#undef CONFIG_BT_DEBUG_CORE
#define CONFIG_BT_DEBUG_CORE 1
#endif

#ifndef CONFIG_BT_ATT_TX_MAX
#define CONFIG_BT_ATT_TX_MAX 1
#endif

#ifndef CONFIG_BT_CONN_TX_MAX
#define CONFIG_BT_CONN_TX_MAX 10
#endif

#ifndef CONFIG_BT_DEVICE_APPEARANCE
#define CONFIG_BT_DEVICE_APPEARANCE 833
#endif

#ifdef CONFIG_BT_MESH

#ifndef CONFIG_BT_MESH_MODEL_KEY_COUNT
#define CONFIG_BT_MESH_MODEL_KEY_COUNT 2
#endif

#ifndef CONFIG_BT_MESH_MODEL_GROUP_COUNT
#define CONFIG_BT_MESH_MODEL_GROUP_COUNT 2
#endif

#ifndef CONFIG_BT_MESH_CRPL
#define CONFIG_BT_MESH_CRPL 5
#endif

#ifndef CONFIG_BT_MESH_ADV_BUF_COUNT
#define CONFIG_BT_MESH_ADV_BUF_COUNT 6
#endif

#ifndef CONFIG_BT_MESH_LABEL_COUNT
#define CONFIG_BT_MESH_LABEL_COUNT 0
#endif

#ifndef CONFIG_BT_MESH_MSG_CACHE_SIZE
#define CONFIG_BT_MESH_MSG_CACHE_SIZE 2
#endif

#ifndef CONFIG_BT_MESH_TX_SEG_MSG_COUNT
#define CONFIG_BT_MESH_TX_SEG_MSG_COUNT 2
#endif

#ifndef CONFIG_BT_MESH_RX_SEG_MAX
#define CONFIG_BT_MESH_RX_SEG_MAX 3
#endif

#ifndef CONFIG_BT_MESH_TX_SEG_MAX
#define CONFIG_BT_MESH_TX_SEG_MAX 3
#endif

#ifndef CONFIG_BT_MESH_RX_SDU_MAX
#define CONFIG_BT_MESH_RX_SDU_MAX 36
#endif

#ifndef CONFIG_BT_MESH_RX_SEG_MSG_COUNT
#define CONFIG_BT_MESH_RX_SEG_MSG_COUNT 2
#endif

#ifndef CONFIG_BT_MESH_SEG_BUFS
#define CONFIG_BT_MESH_SEG_BUFS 64
#endif

#ifndef CONFIG_BT_MESH_ADV_PRIO
#define CONFIG_BT_MESH_ADV_PRIO 5
#endif

#ifndef CONFIG_BT_MESH_PROXY_FILTER_SIZE
#define CONFIG_BT_MESH_PROXY_FILTER_SIZE 1
#endif

#ifndef CONFIG_BT_MESH_NODE_ID_TIMEOUT
#define CONFIG_BT_MESH_NODE_ID_TIMEOUT 60
#endif

#endif /* endof CONFIG_BT_MESH */

#ifdef CONFIG_BT_CTLR

#ifndef CONFIG_BT_CTLR_WORKER_PRIO
#define CONFIG_BT_CTLR_WORKER_PRIO 0
#endif

#ifndef CONFIG_BT_CTLR_JOB_PRIO
#define CONFIG_BT_CTLR_JOB_PRIO 0
#endif

#ifndef CONFIG_BT_CTLR_RX_BUFFERS
#define CONFIG_BT_CTLR_RX_BUFFERS 1
#endif

#ifndef CONFIG_BT_CTLR_XTAL_THRESHOLD
#define CONFIG_BT_CTLR_XTAL_THRESHOLD 5168
#endif

#define CONFIG_BT_RECV_IS_RX_THREAD

#endif /* endof CONFIG_BT_CTLR */
#else
#endif //0
#if defined(__cplusplus)
}
#endif

#endif /* BLE_CONFIG_H */
