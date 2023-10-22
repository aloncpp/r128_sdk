/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY¡¯S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS¡¯SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY¡¯S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __FLASHC_SUN20IW2_H__
#define __FLASHC_SUN20IW2_H__

#include <hal_gpio.h>
/*!< Peripheral memory map */
#define PERIPH_BASE         (0x40000000U)

#define FLASH_CTRL_BASE     (PERIPH_BASE + 0x0000B000)
#define FLASHC_ENC_BASE     (PERIPH_BASE + 0x0003F800)
#define FLASH_MSG_NUM        6

typedef struct flash_pin_msg
{
    gpio_pin_t flash_pin;
    gpio_muxsel_t flash_function;
    gpio_driving_level_t flash_drv_level;
    gpio_pull_status_t flash_pull_state;
    uint8_t keep_pull_state_flags;
} flash_pin_msg;

__attribute__((unused)) static flash_pin_msg flash_nor_sip_msg[FLASH_MSG_NUM] =
{
    {
	/* pin0 */
        .flash_pin = GPIOB(8),
        .flash_function = GPIO_MUXSEL_FUNCTION2,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_UP,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin1 */
        .flash_pin = GPIOB(9),
        .flash_function = GPIO_MUXSEL_FUNCTION2,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_UP,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin2 */
        .flash_pin = GPIOB(10),
        .flash_function = GPIO_MUXSEL_FUNCTION2,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_DOWN_DISABLED,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin3 */
        .flash_pin = GPIOB(11),
        .flash_function = GPIO_MUXSEL_FUNCTION2,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_DOWN_DISABLED,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin4 */
        .flash_pin = GPIOB(12),
        .flash_function = GPIO_MUXSEL_FUNCTION2,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_UP,
        .keep_pull_state_flags = 1,
    },
    {
	/* pin5 */
        .flash_pin = GPIOB(13),
        .flash_function = GPIO_MUXSEL_FUNCTION2,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_DOWN_DISABLED,
        .keep_pull_state_flags = 0,
    },
};

__attribute__((unused)) static flash_pin_msg flash_no_nor_sip_msg[FLASH_MSG_NUM] =
{
    {
	/* pin0 */
        .flash_pin = GPIOB(4),
        .flash_function = GPIO_MUXSEL_FUNCTION5,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_UP,
        .keep_pull_state_flags = 1,
    },
    {
	/* pin1 */
        .flash_pin = GPIOB(5),
        .flash_function = GPIO_MUXSEL_FUNCTION5,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_DOWN_DISABLED,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin2 */
        .flash_pin = GPIOB(6),
        .flash_function = GPIO_MUXSEL_FUNCTION5,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_DOWN_DISABLED,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin3 */
        .flash_pin = GPIOB(7),
        .flash_function = GPIO_MUXSEL_FUNCTION5,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_UP,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin4 */
        .flash_pin = GPIOB(14),
        .flash_function = GPIO_MUXSEL_FUNCTION5,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_UP,
        .keep_pull_state_flags = 0,
    },
    {
	/* pin5 */
        .flash_pin = GPIOB(15),
        .flash_function = GPIO_MUXSEL_FUNCTION5,
        .flash_drv_level = GPIO_DRIVING_LEVEL3,
        .flash_pull_state = GPIO_PULL_DOWN_DISABLED,
        .keep_pull_state_flags = 0,
    },
};


#endif /*__FLASHC_SUN20IW2_H__  */
