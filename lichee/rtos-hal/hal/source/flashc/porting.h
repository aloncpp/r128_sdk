/*
 * =====================================================================================
 *
 *       Filename:  porting.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  2021年03月03日 21时22分16秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef MCU_PORTING_H
#define MCU_PORTING_H

#include <aw_list.h>
#include <hal_mutex.h>
#include <hal_status.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
    #define   __I     volatile             /*!< Defines 'read only' permissions                 */
#else
    #define   __I     volatile const       /*!< Defines 'read only' permissions                 */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions                */
#define     __IO    volatile             /*!< Defines 'read / write' permissions              */

/*
 * Bitwise operation
 */
#define HAL_BIT(pos)                        (1U << (pos))

#define HAL_SET_BIT(reg, mask)              ((reg) |= (mask))
#define HAL_CLR_BIT(reg, mask)              ((reg) &= ~(mask))
#define HAL_GET_BIT(reg, mask)              ((reg) & (mask))
#define HAL_GET_BIT_VAL(reg, shift, vmask)  (((reg) >> (shift)) & (vmask))

#define HAL_MODIFY_REG(reg, clr_mask, set_mask) \
    ((reg) = (((reg) & (~(clr_mask))) | (set_mask)))

/*
 * Macros for accessing LSBs of a 32-bit register (little endian only)
 */
#define HAL_REG_32BIT(reg_addr)  (*((__IO uint32_t *)(reg_addr)))
#define HAL_REG_16BIT(reg_addr)  (*((__IO uint16_t *)(reg_addr)))
#define HAL_REG_8BIT(reg_addr)   (*((__IO uint8_t  *)(reg_addr)))

/* Macro for counting the element number of an array */
#define HAL_ARRAY_SIZE(a)   (sizeof((a)) / sizeof((a)[0]))

/* Wait forever timeout value */
#ifndef HAL_WAIT_FOREVER
#define HAL_WAIT_FOREVER    OS_WAIT_FOREVER
#endif

#define FLASH_SYSLOG    printf
typedef hal_status_t    HAL_Status;

#define FLASH_LOG(flags, fmt, arg...)   \
    do {                                \
        if (flags)                      \
            FLASH_SYSLOG(fmt, ##arg);   \
    } while (0)

#define PCHECK(p)

/* flash debug */
#ifdef CONFIG_BOOTLOADER
#define flash_dbg_mask 0
#else
extern uint8_t flash_dbg_mask;
#endif

#define FLASH_DBG_FLAG	(1 << 0)
#define FLASH_ALE_FLAG	(1 << 1)
#define FLASH_ERR_FLAG 	(1 << 2)
#define FLASH_NWA_FLAG	(1 << 3)

#define FD_DBG_FLAG		(1 << 4)
#define FD_INF_FLAG		(1 << 5)
#define FD_ERR_FLAG		(1 << 6)

#define FLASH_DBG_ON    (flash_dbg_mask & FLASH_DBG_FLAG)
#define FLASH_ALE_ON    (flash_dbg_mask & FLASH_ALE_FLAG)
#define FLASH_ERR_ON    (flash_dbg_mask & FLASH_ERR_FLAG)
#define FLASH_NWA_ON	(flash_dbg_mask & FLASH_NWA_FLAG)

#define FD_DBG_ON		(flash_dbg_mask & FD_DBG_FLAG)
#define FD_ERR_ON		(flash_dbg_mask & FD_ERR_FLAG)
#define FD_INF_ON		(flash_dbg_mask & FD_INF_FLAG)

#define FLASH_DEBUG(fmt, arg...)	FLASH_LOG(FLASH_DBG_ON, fmt"\n", ##arg)
#define FLASH_ALERT(fmt, arg...)	FLASH_LOG(FLASH_ALE_ON, fmt"\n", ##arg)
#define FLASH_ERROR(fmt, arg...)	FLASH_LOG(FLASH_ERR_ON, fmt"\n", ##arg)
#define FLASH_NOWAY(fmt, arg...)	FLASH_LOG(FLASH_NWA_ON, fmt"\n", ##arg)
#define FLASH_NOTSUPPORT()

#if 0
#define FD_DEBUG(fmt, arg...) FLASH_LOG(FD_DBG_ON, "[FD D]: "fmt"\n", ##arg)
#define FD_ERROR(fmt, arg...) FLASH_LOG(FD_ERR_ON, "[FD E]: "fmt"\n", ##arg)
#define FD_INFO(fmt, arg...)  FLASH_LOG(FD_INF_ON, "[FD I]: "fmt"\n", ##arg)
#endif

/* flash controller debug */
#ifdef CONFIG_BOOTLOADER
#define fc_debug_mask 0
#else
extern uint8_t fc_debug_mask;
#endif

#define FC_DBG_FLAG	(1 << 0)
#define FC_WRN_FLAG	(1 << 1)
#define FC_ERR_FLAG	(1 << 2)

#define FC_DBG_ON	(fc_debug_mask & FC_DBG_FLAG)
#define FC_WRN_ON	(fc_debug_mask & FC_WRN_FLAG)
#define FC_ERR_ON	(fc_debug_mask & FC_ERR_FLAG)

#define FC_DEBUG(fmt, arg...) FLASH_LOG(FC_DBG_ON, "[FC D]: "fmt"\n", ##arg)
#define FC_WARN(fmt, arg...)  FLASH_LOG(FC_WRN_ON, "[FC W]: "fmt"\n", ##arg)
#define FC_ERROR(fmt, arg...) FLASH_LOG(FC_ERR_ON, "[FC E]: "fmt"\n", ##arg)


/* xip debug */
extern uint8_t xip_debug_mask;

#define XIP_DBG_FLAG	(1 << 0)
#define XIP_ERR_FLAG	(1 << 1)

#define XIP_DBG_ON		(xip_debug_mask & XIP_DBG_FLAG)
#define XIP_ERR_ON		(xip_debug_mask & XIP_ERR_FLAG)

#define XIP_DEBUG(fmt, arg...) FLASH_LOG(XIP_DBG_ON, "[XIP D]: "fmt"\n", ##arg)
#define XIP_ERROR(fmt, arg...) FLASH_LOG(XIP_ERR_ON, "[XIP E]: "fmt"\n", ##arg)

#define round_div(x, y)	(((x)+((y)/2))/(y))  /* to any y */
#define	nitems(x)	(sizeof((x)) / sizeof((x)[0]))
#define	rounddown(x, y)	(((x)/(y))*(y))
#define	rounddown2(x, y) ((x)&(~((y)-1)))          /* if y is power of two */
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))  /* to any y */
#define	roundup2(x, y)	(((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */
#define powerof2(x)	((((x)-1)&(x))==0)

#define HAL_ASSERT_PARAM(exp)                                           \
    do {                                                                \
        if (!(exp)) {                                                   \
            printf("Invalid param at %s:%d\n", __func__, __LINE__); \
        }                                                               \
    } while (0)

#define HAL_ARRAY_SIZE(a)   (sizeof((a)) / sizeof((a)[0]))

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
}
#endif

#endif
