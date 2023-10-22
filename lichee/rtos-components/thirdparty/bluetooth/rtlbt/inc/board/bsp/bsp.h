/**
********************************************************************************************************
Copyright (c) 2015, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      bsp.h
* @brief     this offer the interface of the bsp.c
* @author    Thomas_li
* @date      2016-10-17
* @version   v0.1
*/
#ifndef _BSP_H_
#define _BSP_H

#ifdef  __cplusplus
extern "C"
{
#endif

void bsp_init(void);
void reset_bt_gpio(void);

void bt_reset_set(void);
void bt_reset_clear(void);

#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif

