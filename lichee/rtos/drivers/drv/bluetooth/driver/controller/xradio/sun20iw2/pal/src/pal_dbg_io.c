/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      Debug GPIO driver implementation.
 *
 *  \author     Xradio BT Team
 *
 *  \Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*************************************************************************************************/

#include "pal_dbg_io.h"
#include <gpio/gpio.h>
#include "hal_time.h"
#include <hal_gpio.h>

#define PAL_DBG_IO_EN   0

/* please select the board you use:
 * R128 VER ANA: 0
 * XR875 VER   : 1
 */
#define CFG_BOARD_ID    0

#if PAL_DBG_IO_EN
typedef struct {
	gpio_pin_t    pin;
	gpio_muxsel_t  fun;
	gpio_driving_level_t  level;
	gpio_pull_status_t  pull;
} GPIO_PinMuxParam;

/**************************************************************************************************
  Functions: Initialization
**************************************************************************************************/
static const GPIO_PinMuxParam g_dbg_gpio[] = {
#if (CFG_BOARD_ID == 0)
	{GPIO_PA0, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//J73.2
	{GPIO_PA6, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//J74.2
	{GPIO_PA28, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//U40.1
	{GPIO_PB14, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//J76.2
	{GPIO_PA15, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//U40.10
	{GPIO_PA18, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//U40.14

	{GPIO_PA7, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG1 J74.1
	{GPIO_PA8, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG2 J75.2
	{GPIO_PA9, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG3 J73.1
	{GPIO_PA10, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG3 J52.RTS
	{GPIO_PA11, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG4 J52.CTS
//	{GPIO_PA12,GPIO_MUXSEL_FUNCTION10,GPIO_DRIVING_LEVEL1,GPIO_PULL_DOWN_DISABLED},//BT_DEBUG5 use for BT_uart_tx
//	{GPIO_PA13,GPIO_MUXSEL_FUNCTION10,GPIO_DRIVING_LEVEL1,GPIO_PULL_DOWN_DISABLED},//BT_DEBUG6 use for BT_uart_rx
	{GPIO_PA14, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG8 U40.15
#endif

#if (CFG_BOARD_ID == 1)
	{GPIO_PA0, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//J74.2
	{GPIO_PA20, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//J74.1
	{GPIO_PA27, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//JM1.5
	{GPIO_PA25, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//JM1.7
	{GPIO_PA28, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//JM1.6
	{GPIO_PA26, GPIO_MUXSEL_OUT, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//JM1.8

	{GPIO_PA7, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG1 J55.9
	{GPIO_PA8, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG2 J71.2
	{GPIO_PA9, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG3 J71.3
	{GPIO_PA10, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG3 J71.4
	{GPIO_PA11, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG4 J71.5
	{GPIO_PA12, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG5 J71.6
	{GPIO_PA13, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG6 J71.7
	{GPIO_PA14, GPIO_MUXSEL_FUNCTION10, GPIO_DRIVING_LEVEL1, GPIO_PULL_DOWN_DISABLED},//BT_DEBUG8 J71.8
#endif
};

/*************************************************************************************************/
/*!
 *  \brief      Initialize Debug GPIO.
 */
/*************************************************************************************************/

void PalIOPinMuxConfig(const GPIO_PinMuxParam *param, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; ++i) {
		hal_gpio_pinmux_set_function(param[i].pin, param[i].fun);
		hal_gpio_set_driving_level(param[i].pin, param[i].level);
		hal_gpio_set_pull(param[i].pin, param[i].pull);
	}
}

void PalIOPinMuxDeConfig(const GPIO_PinMuxParam *param, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; ++i) {
		hal_gpio_pinmux_set_function(param[i].pin, GPIO_MUXSEL_DISABLED);
		hal_gpio_set_driving_level(param[i].pin, GPIO_DRIVING_LEVEL1);
		hal_gpio_set_pull(param[i].pin, GPIO_PULL_DOWN_DISABLED);
	}
}

void PalIoInit(void)
{
	PalIOPinMuxConfig(g_dbg_gpio, sizeof(g_dbg_gpio)/sizeof(GPIO_PinMuxParam));

	PalIoCtl(0xFF, 1);
	PalIoCtl(0xFF, 0);
}

/*************************************************************************************************/
/*!
 *  \brief      De-initialize Debug GPIO.
 */
/*************************************************************************************************/
void PalIoDeInit(void)
{
	PalIoCtl(0xFF, 0);

	PalIOPinMuxDeConfig(g_dbg_gpio, sizeof(g_dbg_gpio)/sizeof(GPIO_PinMuxParam));
}

/*************************************************************************************************/
/*!
 *  \brief      Debug GPIO Controller.
 */
/*************************************************************************************************/
__intr_text
void PalIoCtl(uint8_t io_num, uint8_t io_state)
{
	gpio_data_t data;

	if (io_state == 0) {
		data = GPIO_DATA_LOW;
	} else {
		data = GPIO_DATA_HIGH;
	}
	switch (io_num) {
#if (CFG_BOARD_ID == 0)
	case 0:/* PAL_DBG_IO_ID_H_TASK */
		hal_gpio_set_data(GPIO_PA0, data);
		break;

	case 1:/* PAL_DBG_IO_ID_L_TASK */
		hal_gpio_set_data(GPIO_PA6, data);
		break;

	case 2:/* PAL_DBG_IO_ID_BLE_INTRP */
		hal_gpio_set_data(GPIO_PA28, data);
		break;

	case 3:/* PAL_DBG_IO_ID_ERROR */
		hal_gpio_set_data(GPIO_PB14, data);
		break;

	case 4:/* PAL_DBG_IO_ID_LTASK_BB */
		hal_gpio_set_data(GPIO_PA15, data);
		break;

	case 5:/* PAL_DBG_IO_ID_LTASK_LL */
		hal_gpio_set_data(GPIO_PA18, data);
		break;

	case 0xFF:
		hal_gpio_set_data(GPIO_PA0, data);
		hal_gpio_set_data(GPIO_PA6, data);
		hal_gpio_set_data(GPIO_PA28, data);
		hal_gpio_set_data(GPIO_PB14, data);
		hal_gpio_set_data(GPIO_PA15, data);
		hal_gpio_set_data(GPIO_PA18, data);
		hal_gpio_set_data(GPIO_PA7, data);
		hal_gpio_set_data(GPIO_PA8, data);
		hal_gpio_set_data(GPIO_PA9, data);
		hal_gpio_set_data(GPIO_PA10, data);
		hal_gpio_set_data(GPIO_PA11, data);
		hal_gpio_set_data(GPIO_PA14, data);
	default:
		break;
#endif

#if (CFG_BOARD_ID == 1)
	case 0:/* PAL_DBG_IO_ID_H_TASK */
		hal_gpio_set_data(GPIO_PA0, data);
		break;

	case 1:/* PAL_DBG_IO_ID_L_TASK */
		hal_gpio_set_data(GPIO_PA20, data);
		break;

	case 2:/* PAL_DBG_IO_ID_BLE_INTRP */
		hal_gpio_set_data(GPIO_PA27, data);
		break;

	case 3:/* PAL_DBG_IO_ID_ERROR */
		hal_gpio_set_data(GPIO_PA25, data);
		break;

	case 4:/* PAL_DBG_IO_ID_LTASK_BB */
		hal_gpio_set_data(GPIO_PA28, data);
		break;

	case 5:/* PAL_DBG_IO_ID_LTASK_LL */
		hal_gpio_set_data(GPIO_PA26, data);
		break;

	case 0xFF:
		hal_gpio_set_data(GPIO_PA0, data);
		hal_gpio_set_data(GPIO_PA20, data);
		hal_gpio_set_data(GPIO_PA27, data);
		hal_gpio_set_data(GPIO_PA25, data);
		hal_gpio_set_data(GPIO_PA28, data);
		hal_gpio_set_data(GPIO_PA26, data);
		hal_gpio_set_data(GPIO_PA7, data);
		hal_gpio_set_data(GPIO_PA8, data);
		hal_gpio_set_data(GPIO_PA9, data);
		hal_gpio_set_data(GPIO_PA10, data);
		hal_gpio_set_data(GPIO_PA11, data);
		hal_gpio_set_data(GPIO_PA12, data);
		hal_gpio_set_data(GPIO_PA13, data);
		hal_gpio_set_data(GPIO_PA14, data);
	default:
		break;

#endif

	}
}

/*************************************************************************************************/
/*!
 *  \brief      Set GPIO up.
 *
 *  \param      Id           GPIO ID.
 */
/*************************************************************************************************/
__intr_text
void PalIoOn(uint8_t Id)
{
	PalIoCtl(Id, 1);
}

/*************************************************************************************************/
/*!
 *  \brief      Set GPIO down.
 *
 *  \param      Id           GPIO ID.
 */
/*************************************************************************************************/
__intr_text
void PalIoOff(uint8_t Id)
{
	PalIoCtl(Id, 0);
}

/*************************************************************************************************/
/*!
 *  \brief      Set GPIO up and down.
 *
 *  \param      Id           GPIO ID.
 *  \param      delay        delay time.
 */
/*************************************************************************************************/

__intr_text
void PalIoOnAndOff(uint8_t Id, uint8_t delay)
{
	PalIoCtl(Id, 1);
	hal_udelay(delay);
	PalIoCtl(Id, 0);
}

#else

void PalIoInit(void)
{

}

void PalIoDeInit(void)
{

}

/* Control and Status */
void PalIoCtl(uint8_t io_num, uint8_t io_state)
{

}

void PalIoOn(uint8_t id)
{

}

void PalIoOff(uint8_t id)
{

}

void PalIoOnAndOff(uint8_t id, uint8_t delay)
{

}

#endif
