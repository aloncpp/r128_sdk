/**
  ******************************************************************************
  * @file    rtwlan_bsp.h
  * @author  Realtek software team
  * @version V0.1
  * @date    05-March-2013
  * @brief   Realtek WLAN hardware configuration.
  ******************************************************************************
  */

#ifndef __REALTEK_WLAN_BSP_H__
#define __REALTEK_WLAN_BSP_H__
/* Includes ---------------------------------------------------------------*/


/* Exported functions ------------------------------------------------------- */
void Set_WLAN_Power_On(void);
void Set_WLAN_Power_Off(void);
int WLAN_BSP_Config(void);
void WLAN_BSP_UsLoop(int us);
unsigned long WLAN_BSP_Transfer(unsigned char* buf, unsigned int buf_len);

#endif// __REALTEK_WLAN_BSP_H__
