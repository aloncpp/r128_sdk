/* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.

 * Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
 * the the People's Republic of China and other countries.
 * All Allwinner Technology Co.,Ltd. trademarks are used with permission.

 * DISCLAIMER
 * THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
 * IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
 * IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
 * ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
 * ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
 * COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
 * YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.


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

#ifndef SYSCTL_SUN20IW2_H
#define SYSCTL_SUN20IW2_H

#ifdef __cplusplus
extern "C" {
#endif

#define SYSCTL_BASE         (0x4000A000U)

/**
 * @brief SYSCTL register block structure
 */
typedef struct {
         uint32_t RESERVED0[17];     /* offset: 0x00 */
         uint32_t SIP_TEST_MAP;      /* offset: 0x44 */
         uint32_t RESERVED1;         /* offset: 0x48 */
         uint32_t RAM_REMAP_REG;     /* offset: 0x4C */
         uint32_t PS_CTL_REG;        /* offset: 0x50 */
         uint32_t PS_CNT_REG;        /* offset: 0x54 */
         uint32_t RESERVED58[2];     /* offset: 0x58 */
         uint32_t FiveV_IO_BIAS_MODE;   /* offset: 0x60 */
         uint32_t SMC_CRY_CONFIG_REG;   /* offset: 0x64 */
         uint32_t SMC_CRY_KEY_REG;   /* offset: 0x68 */
         uint32_t SMC_CRY_EN_REG;    /* offset: 0x6C */
         uint32_t RESERVED64[16];    /* offset: 0x70 */
         uint32_t GENRAL_DBG_REG[6]; /* offset: 0xB0 */
} SYSCTL_T;

#define SYSCTL ((SYSCTL_T *)SYSCTL_BASE) /* address: 0x4000A000 */

/* SYSCTL->SIP_TEST_MAP 0x44 */
#define SYSCTL_SIP_TEST_MAP_WRITE_SHIFT          (16)
#define SYSCTL_SIP_TEST_MAP_WRITE_EN             (0x429D << SYSCTL_SIP_TEST_MAP_WRITE_SHIFT)
#define SYSCTL_SIP_FLASH_TEST_MAP_SHIFT          (0)
#define SYSCTL_SIP_FLASH_TEST_MAP_EN             (0x1 << SYSCTL_SIP_FLASH_TEST_MAP_SHIFT)


/* SYSCTL->RAM_REMAP_REG 0x4C */
#define SYSCTL_BT_RAM_REMAP_SHIFT        (4)
#define SYSCTL_BT_RAM_REMAP_MASK          (0x1 << SYSCTL_BT_RAM_REMAP_SHIFT)

#define SYSCTL_CSI_RAM_REMAP_SHIFT       (3)
#define SYSCTL_CSI_RAM_REMAP_MASK          (0x1 << SYSCTL_CSI_RAM_REMAP_SHIFT)

#define SYSCTL_ROM_SECURE_SEL_SHIFT       (2)
#define SYSCTL_ROM_SECURE_SEL_MASK          (0x1 << SYSCTL_ROM_SECURE_SEL_SHIFT)

#define SYSCTL_BSP_BOOT_SRAM_REMAP_EN_SHIFT    (0)
#define SYSCTL_BSP_BOOT_SRAM_REMAP_EN_MASK     (0x1 << SYSCTL_BSP_BOOT_SRAM_REMAP_EN_SHIFT)


/* SYSCTL->PS_CTL_REG 0x50 */
#define SYSCTL_CLK250M_CNT_RDY_SHIFT    (16)
#define SYSCTL_CLK250M_CNT_RDY_MASK     (0x1 << SYSCTL_CLK250M_CNT_RDY_SHIFT)

#define SYSCTL_DEVICE_SEL_SHIFT         (8)
#define SYSCTL_DEVICE_SEL_MASK          (0x7 << SYSCTL_DEVICE_SEL_SHIFT)

typedef enum {
	SYSCTL_PSENSOR_0                = (0U << SYSCTL_DEVICE_SEL_SHIFT),
	SYSCTL_PSENSOR_1                = (1U << SYSCTL_DEVICE_SEL_SHIFT),
} SYSCTL_PsensorId;

#define SYSCTL_PS_EN_SEL_SHIFT          (4)
#define SYSCTL_PS_EN_SEL_MASK           (0x3 << SYSCTL_PS_EN_SEL_SHIFT)

typedef enum {
	SYSCTL_OSC_DISABLE              = (0U  << SYSCTL_PS_EN_SEL_SHIFT),
	SYSCTL_OSC_RVT_CASECODE         = (1U  << SYSCTL_PS_EN_SEL_SHIFT),
	SYSCTL_OSC_LVT_CASECODE         = (2U  << SYSCTL_PS_EN_SEL_SHIFT),
	SYSCTL_OSC_RVT_NORMAL           = (3U  << SYSCTL_PS_EN_SEL_SHIFT),
} SYSCTL_OSCSelect;

#define SYSCTL_PS_N_PRD_SHIFT           (1)
#define SYSCTL_PS_N_PRD_MASK            (0x7 << SYSCTL_PS_N_PRD_SHIFT)
#define SYSCTL_PS_N_PRD_VAL(v)          ((v & 0x7) << SYSCTL_PS_N_PRD_SHIFT)

#define SYSCTL_PS_EN_SHIFT              (0)
#define SYSCTL_PS_EN_MASK               (0x1 << SYSCTL_PS_EN_SHIFT)


/* SYSCTL->SMC_CRY_EN_REG 0x6C */
#define SYSCTL_SMC_CRY_EN_SHIFT    (0)
#define SYSCTL_SMC_CRY_EN_MASK     (0x1 << SYSCTL_SMC_CRY_EN_SHIFT)

#ifdef __cplusplus
}
#endif
#endif  /*SYSCTL_SUN20IW2_H*/
