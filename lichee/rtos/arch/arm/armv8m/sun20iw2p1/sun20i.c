#include <serial.h>
#include <interrupt.h>
#include <stdio.h>
#include <stdint.h>
#include <FreeRTOS.h>
#include <string.h>
#include <task.h>
#include <aw_version.h>
#include <irqs.h>
#include <platform.h>
#include <memory.h>
#include <core_cm33.h>
#include <io.h>
#include <hal_gpio.h>
#include <hal_uart.h>
#include <hal_msgbox.h>
#include <hal_clk.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <hal_timer.h>
#include <hal_hwspinlock.h>
#include <sunxi_hal_watchdog.h>
#include <sunxi_hal_rtc.h>
#include <private_rtos.h>
#include <sunxi_hal_rcosc_cali.h>

#ifdef CONFIG_DRIVERS_PRCM
#include <hal_prcm.h>
#endif

#include <compiler.h>

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

#ifdef CONFIG_COMPONENTS_PM
#include "pm_init.h"
#endif

#ifdef CONFIG_DRIVER_SYSCONFIG
#include <hal_cfg.h>
#include <script.h>
#endif

extern const unsigned char __VECTOR_BASE[];

extern void timekeeping_init(void);

__weak void sunxi_dma_init(void)
{
    return;
}

__weak void sunxi_gpio_init(void)
{
    return;
}

__weak int sunxi_soundcard_init(void)
{
    return 0;
}

__weak void heap_init(void)
{
    return;
}

static int cpu_rate;

static int clk_ldo_enable(void)
{
	hal_clk_t clk;
	hal_clk_status_t ret = 0;

	/*enable clk_ldo1*/
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_LDO1_EN);
	ret = hal_clock_enable(clk);
	if (HAL_CLK_STATUS_OK != ret) {
		hal_clock_put(clk);
		ret = -1;
		return ret;
	}

	/*enable clk_ldo2*/
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_LDO2_EN);
	ret = hal_clock_enable(clk);
	if (HAL_CLK_STATUS_OK != ret) {
		hal_clock_put(clk);
		ret = -1;
		return ret;
	}

	return ret;
}

static int dpll1_clk_set(u32 freq)
{
	hal_clk_t clk;
	hal_clk_status_t ret = 0;

	/*set dpll1_clk*/
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_DPLL1);
	ret = hal_clock_enable(clk);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err;
	}

	ret = hal_clk_set_rate(clk, freq);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err;
	}

err:
	hal_clock_put(clk);
	return ret;
}

static int dpll2_clk_set(u32 freq)
{
	hal_clk_t clk;
	hal_clk_status_t ret = 0;

	/*set dpll2_clk*/
	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_DPLL2);
	ret = hal_clock_enable(clk);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err;
	}

	ret = hal_clk_set_rate(clk, freq);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err;
	}

err:
	hal_clock_put(clk);
	return ret;
}

static int dpll3_clk_set(u32 freq)
{
	hal_clk_t clk;
	hal_clk_status_t ret = 0;

	clk = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_DPLL3);
	ret = hal_clock_enable(clk);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err;
	}

	ret = hal_clk_set_rate(clk, freq);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err;
	}

err:
	hal_clock_put(clk);
	return ret;
}

/*
 * we fixed ck1_m33 to 384M,
 * so freq only support 384M,192M,128M,96M,64M,48M,32M,24M
 */
static int ar200a_clk_set(u32 freq)
{
	u32 rate;
	hal_clk_t clk_ck1_m33, clk_ck_m33, clk_sys, clk_ar200a_f;
	hal_clk_status_t ret = 0;

	/*set ck1_m33_clk*/
	//sr32(CCMU_AON_BASE+0xa4,  3, 1, 1);
	//0x4004c4a4: 0x80800008
	clk_ck1_m33 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK1_M33);
	ret = hal_clock_enable(clk_ck1_m33);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err1;
	}

	/*fixed 384M*/
	//sr32(CCMU_AON_BASE+0xa4,  0, 3, 0x3);
	//0x4004c4a4: 0x8080000b
	ret = hal_clk_set_rate(clk_ck1_m33, 384000000);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err2;
	}

	/*set ck_m33_clk*/
	//sr32(CCMU_AON_BASE+0xe0, 19, 1, 0);
	//0x4004c4e0: 0x00000101
	clk_ck_m33 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK_M33);
	ret = hal_clk_set_parent(clk_ck_m33, clk_ck1_m33);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err3;
	}

	/*set sys_clk*/
	//sr32(CCMU_AON_BASE+0xe0,  8, 4, 0x7);
	//0x4004c4e0: 0x00000701
	clk_sys = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_SYS);
	ret = hal_clk_set_rate(clk_sys, freq);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err4;
	}

	/*set ar200a_f_clk*/
	//sr32(CCMU_AON_BASE+0xe0, 12, 2, 0x2);
	//0x4004c4e0: 0x00002701
	clk_ar200a_f = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_AR200A_F);
	ret = hal_clk_set_parent(clk_ar200a_f, clk_sys);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err5;
	}
	rate = hal_clk_recalc_rate(clk_ar200a_f);
	cpu_rate = rate;

err5:
	hal_clock_put(clk_ar200a_f);
err4:
	hal_clock_put(clk_sys);
err3:
	hal_clock_put(clk_ck_m33);
err2:
err1:
	hal_clock_put(clk_ck1_m33);
	return ret;
}

/*
 * we fixed ck1_dev to 384M,
 * so freq only support 384M,192M,128M,96M,64M,48M,32M,24M
 */
static int device_clk_set(u32 freq)
{
	u32 rate;
	hal_clk_t clk_ck1_dev, clk_ck_dev, clk_device;
	hal_clk_status_t ret = 0;

	/*set ck1_dev_clk*/
	//sr32(CCMU_AON_BASE+0xa4, 23, 1, 1);
	//0x4004c4a4: 0x8080000b
	clk_ck1_dev = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK1_DEV);
	ret = hal_clock_enable(clk_ck1_dev);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err1;
	}

	/*fixed 384M*/
	//sr32(CCMU_AON_BASE+0xa4, 20, 2, 0x2);
	//0x4004c4a4: 0x80a0000b
	ret = hal_clk_set_rate(clk_ck1_dev, 384000000);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err2;
	}

	/*set ck_dev_clk*/
	//sr32(CCMU_AON_BASE+0xe0, 16, 1, 0);
	//0x4004c4e0: 0x00002701
	clk_ck_dev = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK_DEV);
	ret = hal_clk_set_parent(clk_ck_dev, clk_ck1_dev);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err3;
	}

	/*set device_clk*/
	//sr32(CCMU_AON_BASE+0xe0,  0, 4, 0x7);
	//0x4004c4e0: 0x00002707
	clk_device = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_DEVICE);
	ret = hal_clk_set_rate(clk_device, freq);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err4;
	}

	rate = hal_clk_recalc_rate(clk_device);
err4:
	hal_clock_put(clk_device);
err3:
	hal_clock_put(clk_ck_dev);
err2:
err1:
	hal_clock_put(clk_ck1_dev);
	return ret;
}

enum {
	APB_FREQ_AHB_DIV1 = 0,
	APB_FREQ_AHB_DIV2,
	APB_FREQ_AHB_DIV4,
	APB_FREQ_AHB_DIV8,
};

static int apb_clk_set_div(u32 div)
{
	/*set src to ahb*/
	sr32(CCMU_AON_BASE+0xe0,  6, 2, 0x2);

	/*set div*/
	switch (div) {
	case APB_FREQ_AHB_DIV1:
	case APB_FREQ_AHB_DIV2:
	case APB_FREQ_AHB_DIV4:
	case APB_FREQ_AHB_DIV8:
		sr32(CCMU_AON_BASE+0xe0,  4, 2, div);
		break;
	default:
		return -1;
	};

	return 0;
}

typedef struct
{
	const char *mux_name;
	hal_clk_type_t mux_clk_type;
	hal_clk_id_t mux_clk_id;
	hal_clk_type_t parent_clk_type;
	hal_clk_id_t parent_clk_id;
} mux_clk_config_para_t;

static int mux_clk_set_parent(mux_clk_config_para_t *para)
{
	int ret;

	hal_clk_t mux_clk, parent_clk;
	mux_clk = hal_clock_get(para->mux_clk_type, para->mux_clk_id);
	if (!mux_clk)
	{
		printf("get the mux clk(name: '%s', type: %d, id: %d) failed\n", para->mux_name, para->mux_clk_type, para->mux_clk_id);
		return -1;
	}

	parent_clk = hal_clock_get(para->parent_clk_type, para->parent_clk_id);
	if (!parent_clk)
	{
		printf("get the parent clk(type: %d, id: %d) of mux clk(%s) failed\n",
			para->parent_clk_type, para->parent_clk_id, para->mux_name);
		return -2;
	}

	ret = hal_clock_enable(parent_clk);
	if (HAL_CLK_STATUS_OK != ret)
	{
		printf("enable the parent clk(type: %d, id: %d) of mux clk(%s) failed\n",
			para->parent_clk_type, para->parent_clk_id, para->mux_name);
		return -3;
	}

	ret = hal_clk_set_parent(mux_clk, parent_clk);
	if (HAL_CLK_STATUS_OK != ret)
	{
		printf("set the parent clk(type: %d, id: %d) of mux clk(%s) failed\n",
			para->parent_clk_type, para->parent_clk_id, para->mux_name);
		return -4;
	}

	return 0;
}

static int lf_clk_set_parent(hal_clk_type_t parent_clk_type, hal_clk_id_t parent_clk_id)
{
	mux_clk_config_para_t config_para;
	config_para.mux_name = "LF MUX";
	config_para.mux_clk_type = HAL_SUNXI_R_CCU;
	config_para.mux_clk_id = CLK_LF_SEL;
	config_para.parent_clk_type = parent_clk_type;
	config_para.parent_clk_id = parent_clk_id;

	return mux_clk_set_parent(&config_para);
}

static int sys_32k_set_parent(hal_clk_type_t parent_clk_type, hal_clk_id_t parent_clk_id)
{
	mux_clk_config_para_t config_para;
	config_para.mux_name = "SYS_32K MUX";
	config_para.mux_clk_type = HAL_SUNXI_R_CCU;
	config_para.mux_clk_id = CLK_SYS_32K_SEL;
	config_para.parent_clk_type = parent_clk_type;
	config_para.parent_clk_id = parent_clk_id;

	return mux_clk_set_parent(&config_para);
}

static int sysrtc_32k_set_parent(hal_clk_type_t parent_clk_type, hal_clk_id_t parent_clk_id)
{
	mux_clk_config_para_t config_para;
	config_para.mux_name = "SYSRTC_32K MUX";
	config_para.mux_clk_type = HAL_SUNXI_R_CCU;
	config_para.mux_clk_id = CLK_SYSRTC32K;
	config_para.parent_clk_type = parent_clk_type;
	config_para.parent_clk_id = parent_clk_id;

	return mux_clk_set_parent(&config_para);
}

static int sysrtc_32k_set_defalut_parent(void)
{
	sysrtc_32k_set_parent(HAL_SUNXI_R_CCU, CLK_LF_SEL);
}

static int ble_32k_set_parent(hal_clk_type_t parent_clk_type, hal_clk_id_t parent_clk_id)
{
	mux_clk_config_para_t config_para;
	config_para.mux_name = "BLE_32K MUX";
	config_para.mux_clk_type = HAL_SUNXI_R_CCU;
	config_para.mux_clk_id = CLK_BLE_SEL;
	config_para.parent_clk_type = parent_clk_type;
	config_para.parent_clk_id = parent_clk_id;

	return mux_clk_set_parent(&config_para);
}

static void disable_rc_lf_div_clk_freq_detection(void)
{
	uint32_t reg_addr, reg_data;
	reg_addr = 0x40050000 + 0x140;
	reg_data = readl(reg_addr);
	reg_data &= ~(1 << 0);
	writel(reg_data, reg_addr);
}

static void rcosc_init(void)
{
	RCOCALI_InitParam cal_para;
	cal_para.cnt_n = 8192;
	cal_para.out_clk = 32000;
	HAL_RcoscCali_Init(&cal_para);
	RCOCALI_ConfigParam configParm = {
		.mode = PRCM_RCOSC_WK_MODE_SEL_SCALE,
		.phase2_times = PRCM_RCOSC_SCALE_PHASE2_WK_TIMES_4,
		.phase3_times = PRCM_RCOSC_SCALE_PHASE3_WK_TIMES_24,
		.phase1_num = 1,
		.phase2_num = 1,
		.wup_time = 640,
	};
	HAL_RcoscCali_Config(&configParm);
	HAL_RcoscCali_Start();

}

#define DPLL1_FREQ     (1920000000)
#define DPLL2_FREQ     (1920000000)
#define DPLL3_FREQ     (1600000000)
#define AR200A_FREQ    ( 192000000)
#define DEVICE_FREQ    ( 192000000)
static int sys_clk_init(void)
{
	/* enable clk_ldo */
	clk_ldo_enable();

	/*set DPLL1 to 1920M*/
	dpll1_clk_set(DPLL1_FREQ);
	dpll2_clk_set(DPLL2_FREQ);
	dpll3_clk_set(DPLL3_FREQ);

	/* select wlan 160M clock src by RFIP0_DPLL/RFIP1_DPLL */
#if defined (CONFIG_WLAN_CLK_BY_RFIP0_DPLL)
	sr32(CCMU_AON_BASE + 0xc4, 2, 1, 0);
#elif defined(CONFIG_WLAN_CLK_BY_RFIP1_DPLL)
	sr32(CCMU_AON_BASE + 0xc4, 2, 1, 1);
#endif

	/* select bt 192M clock src by RFIP0_DPLL/RFIP1_DPLL */
#if defined (CONFIG_BT_CLK_BY_RFIP0_DPLL)
	sr32(CCMU_AON_BASE + 0xc4, 1, 1, 0);
#elif defined(CONFIG_BT_CLK_BY_RFIP1_DPLL)
	sr32(CCMU_AON_BASE + 0xc4, 1, 1, 1);
#endif

	/*only support 384M,192M,128M,96M,64M,48M,32M,24M*/
	ar200a_clk_set(AR200A_FREQ);

	/*only support 384M,192M,128M,96M,64M,48M,32M,24M*/
	device_clk_set(DEVICE_FREQ);

	/* apb frequency support ahb, ahb/2, ahb/4, ahb/8,
	 * and ahb is same as ar200a*/
	apb_clk_set_div(APB_FREQ_AHB_DIV2);

#ifdef CONFIG_USE_EXTERNAL_LOW_FREQ_CRYSTAL_CLK
	lf_clk_set_parent(HAL_SUNXI_R_CCU, CLK_OSC32K_EN);
#else
	lf_clk_set_parent(HAL_SUNXI_R_CCU, CLK_RCOSC_DIV_32K);
#endif

	/* don't set sys_32k's clock source to rccal output clock when enable wifi hardware,
	 * otherwise the wifi hardware will wakeup abnormally */
#ifdef CONFIG_USE_EXTERNAL_LOW_FREQ_CRYSTAL_CLK
	sys_32k_set_parent(HAL_SUNXI_R_CCU, CLK_LF_SEL);
#else

#ifdef CONFIG_DRIVERS_XRADIO
	sys_32k_set_parent(HAL_SUNXI_R_CCU, CLK_LF_SEL);
#else
	sys_32k_set_parent(HAL_SUNXI_R_CCU, CLK_RCCAL32K);
#endif

#endif

#ifdef CONFIG_USE_EXTERNAL_LOW_FREQ_CRYSTAL_CLK
	sysrtc_32k_set_parent(HAL_SUNXI_R_CCU, CLK_LF_SEL);
	ble_32k_set_parent(HAL_SUNXI_R_CCU, CLK_LF_SEL);
#else
	sysrtc_32k_set_parent(HAL_SUNXI_R_CCU, CLK_RCCAL32K);
	ble_32k_set_parent(HAL_SUNXI_R_CCU, CLK_RCCAL32K);
#endif

	/* for save power dissipation, disable the frequency detection for RC_LF/32 */
	disable_rc_lf_div_clk_freq_detection();
	return 0;
}

static void set_vbat_mon_ctrl(void)
{
	HAL_PRCM_SetVBATMonLowVoltage(2700);
	HAL_PRCM_SetVBATMonSwModeEnable(1); //bit1
	HAL_PRCM_SetVBATMonitorEnable(1); //bit0
	HAL_PRCM_SetVBATLowVoltMode(PRCM_VBAT_LOW_VOL_MODE_RESET); //bit2
}

static void prvSetupHardware( void )
{
    int timer_reload_val = cpu_rate / configTICK_RATE_HZ; /* 1ms */

    irq_init();

    SysTick_Config(timer_reload_val);

    hal_clock_init();
    sysrtc_32k_set_defalut_parent();
    rcosc_init();
    sys_clk_init();
    timekeeping_init();

#ifdef CONFIG_POWER_FALL_PROTECTION
    set_vbat_mon_ctrl();
#endif

#ifdef CONFIG_DRIVERS_HWSPINLOCK
    hal_hwspinlock_init();
#endif

    hal_gpio_init();
#if !defined(CONFIG_DISABLE_ALL_UART_LOG)
    serial_init();
#if !defined(CONFIG_COMPONENT_CLI)
    hal_uart_disable_rx(CONFIG_CLI_UART_PORT);
#endif
#endif

#ifdef CONFIG_DRIVERS_RTC
    hal_rtc_init();
#endif
#ifdef CONFIG_DRIVERS_WATCHDOG
    hal_watchdog_init();
#endif
#ifdef CONFIG_DRIVERS_DMA
    hal_dma_init();
#endif
#ifdef CONFIG_HPSRAM_INIT_IN_OS
    hpsram_init();
#endif
#ifdef CONFIG_DRIVERS_MSGBOX_AMP
    hal_msgbox_init();
#endif
#ifdef CONFIG_DRIVERS_SOUND
    sunxi_soundcard_init();
#endif
}

void systeminit(void)
{
	/*  setting vector table relocation  */
	SCB->VTOR = (uint32_t)__VECTOR_BASE; /*  Vector Table Relocation in Internal SRAM. */

	/*  Enable some system fault exceptions */
	SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk;
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
	SCB->CCR &= ~SCB_CCR_UNALIGN_TRP_Msk;
	NVIC_SetPriorityGrouping(0x4);
}

#ifdef CONFIG_ARCH_ARMV8M_DEFAULT_BOOT_RISCV

#define RISCV_DPLL1_CLK 1
#define RISCV_DPLL3_CLK 3

#if defined(CONFIG_PROJECT_R128S1) || defined(CONFIG_PROJECT_R128S2) || defined(CONFIG_PROJECT_R128S3)
#define RISCV_PLL_CLK_SOURCE RISCV_DPLL3_CLK
#define RISCV_PLL_CLK_FREQ	(533333334)
#define RISCV_DIV_CLK_FREQ	(533333334)
#elif defined(CONFIG_PROJECT_XR875) || defined(CONFIG_PROJECT_XR875S1)
#define RISCV_PLL_CLK_SOURCE RISCV_DPLL3_CLK
#define RISCV_PLL_CLK_FREQ	(533333334)
#define RISCV_DIV_CLK_FREQ	(533333334)
#endif

int sun20i_boot_c906(void)
{
	printf("---boot c906---\n");

	hal_clk_t clk_ck1_c906, clk_ck_c906, clk_c906_sel, clk_c906_gate, clk_c906_div;
	hal_clk_t clk_c906_cfg;
#if (RISCV_PLL_CLK_SOURCE == RISCV_DPLL3_CLK)
	hal_clk_t clk_ck3_c906;
#endif
	struct reset_control *c906_cfg_rst, *c906_apb_soft_rst, *c906_core_rst;
	hal_clk_status_t ret = 0;

	//rv wakeup enable
	sr32(PMC_BASE + 0x100, 8, 1, 1);
	//wait rv alive
	while ((readl(PMC_BASE + 0x104) & (1 << 8)) == 0);

	//enable clk_ck1_c906
	//sr32(CCMU_AON_BASE+0xa4,  7, 1, 0x1);
	clk_ck1_c906 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK1_C906);
	ret = hal_clock_enable(clk_ck1_c906);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err1;
	}

#if (RISCV_PLL_CLK_SOURCE == RISCV_DPLL3_CLK)
	clk_ck3_c906 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK3_C906);
	ret = hal_clock_enable(clk_ck3_c906);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto exit_with_put_dpll3_clk;
	}

	ret = hal_clk_set_rate(clk_ck3_c906, RISCV_PLL_CLK_FREQ);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto exit_with_put_dpll3_clk;
	}
#endif

#if (RISCV_PLL_CLK_SOURCE == RISCV_DPLL1_CLK)
	//set clk_ck1_c906 clk to 480M
	//sr32(CCMU_AON_BASE+0xa4,  4, 3, 0x1);
	//set clk_ck1_c906 clk to 640M
	//sr32(CCMU_AON_BASE+0xa4,  4, 3, 0x2);
	ret = hal_clk_set_rate(clk_ck1_c906, RISCV_PLL_CLK_FREQ);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err2;
	}
#endif

	/*set clk_ck_c906 source to clk_ck1_c906*/
	//sr32(CCMU_AON_BASE+0xe0, 17, 1, 0x0);
	clk_ck_c906 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CKPLL_C906_SEL);
#if (RISCV_PLL_CLK_SOURCE == RISCV_DPLL1_CLK)
	ret = hal_clk_set_parent(clk_ck_c906, clk_ck1_c906);
#endif
#if (RISCV_PLL_CLK_SOURCE == RISCV_DPLL3_CLK)
	ret = hal_clk_set_parent(clk_ck_c906, clk_ck3_c906);
#endif
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err3;
	}

	//set clk_c906_sel source to clk_ck_c906
	//sr32(CCMU_BASE+0x064, 4, 2, 0x2);
	clk_c906_sel = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_SEL);
	ret = hal_clk_set_parent(clk_c906_sel, clk_ck_c906);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err4;
	}

	//set clk_c906_sel source to clk_ck_c906
	//sr32(CCMU_BASE+0x064,31,1,1); //rv clk enable
	clk_c906_gate = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_GATE);
	ret = hal_clock_enable(clk_c906_gate);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err5;
	}

	//set clk_ck_c906_div to 480000000 or 640000000;
	//sr32(CCMU_BASE+0x064, 0, 2, 0x0);
	clk_c906_div = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_DIV);

	ret = hal_clk_set_rate(clk_c906_div, RISCV_DIV_CLK_FREQ);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err6;
	}

	printf("C906 CPU Freq: %d MHz\n", hal_clk_recalc_rate(clk_c906_div) / 1000 / 1000);

	//rv clk gating
	//sr32(CCMU_BASE+0x014,19,1,1);
	clk_c906_cfg = hal_clock_get(HAL_SUNXI_CCU, CLK_RISCV_CFG);
	ret = hal_clock_enable(clk_c906_cfg);
	if (HAL_CLK_STATUS_OK != ret) {
		ret = -1;
		goto err7;
	}

	//rv clk rst
	//sr32(CCMU_BASE+0x018,19,1,1);
	c906_cfg_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_RISCV_CFG);
	if (hal_reset_control_deassert(c906_cfg_rst)) {
		ret = -1;
		goto err8;
	}

	//rv sys apb soft rst
	//sr32(CCMU_BASE+0x018,21,1,1);
	c906_apb_soft_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_RISCV_SYS_APB_SOFT);
	if (hal_reset_control_deassert(c906_apb_soft_rst)) {
		ret = -1;
		goto err9;
	}

	//rv start address
#if CONFIG_ARCH_RISCV_START_ADDRESS
	put_wvalue(RISCV_CFG_BASE + 0x004, CONFIG_ARCH_RISCV_START_ADDRESS);
#else
	put_wvalue(RISCV_CFG_BASE + 0x004, rtos_spare_head.reserved[0]);
#endif

	//rv core reset
	//sr32(CCMU_BASE+0x18,16,1,1);
	c906_core_rst = hal_reset_control_get(HAL_SUNXI_RESET, RST_RISCV_CORE);
	if (hal_reset_control_deassert(c906_core_rst)) {
		ret = -1;
	}
err10:
	hal_reset_control_put(c906_core_rst);
err9:
	hal_reset_control_put(c906_apb_soft_rst);
err8:
	hal_reset_control_put(c906_cfg_rst);
err7:
	hal_clock_put(clk_c906_cfg);
err6:
	hal_clock_put(clk_c906_div);
err5:
	hal_clock_put(clk_c906_gate);
err4:
	hal_clock_put(clk_c906_sel);
err3:
	hal_clock_put(clk_ck_c906);

#if (RISCV_PLL_CLK_SOURCE == RISCV_DPLL3_CLK)
exit_with_put_dpll3_clk:
	hal_clock_put(clk_ck3_c906);
#endif

err2:
err1:
	hal_clock_put(clk_ck1_c906);
	return ret;
}
#endif

#if defined(CONFIG_ARCH_ARMV8M_DEFAULT_BOOT_DSP) || defined(CONFIG_COMMAND_BOOT_DSP) \
	|| defined(CONFIG_PM_SUBSYS_DSP_SUPPORT)
#define DSP_CORE_CLOCK_FREQ (400000000)
#define DSP_LDO_WORK_VOLT (1200) //400M@1.2V, 274M@1.1V
int __sun20i_boot_dsp_with_start_addr(uint32_t dsp_start_addr)
{
    hal_clk_t clk_ck1_hifi5, clk_ck3_hifi5, clk_ck_hifi5;
    hal_clk_t clk_hifi5_mux, clk_hifi5_div, clk_hifi5_core;
    hal_clk_t clk_hifi5_cfg;
    struct reset_control *rst_hifi5_cfg, *rst_hifi5_debug, *rst_hifi5_core;
    hal_clk_status_t ret = 0;

#define CCMU_DSP_RST_REG (CCMU_BASE+0x18)
#define CCMU_DSP_GAT_REG (CCMU_BASE+0x14)

#define DSP0_CTRL_REG0	(DSP_CFG_BASE+0x04)
#define DSP0_ALTRST_REG (DSP_CFG_BASE+0x00)

#ifdef CONFIG_DRIVERS_PRCM
    HAL_PRCM_SetDSPLDOSlpVoltage(DSP_LDO_WORK_VOLT);
#else
#define GPRCM_DSP_LDO_CTRL_REG (0x40050000 + 0x4C)
    sr32(GPRCM_DSP_LDO_CTRL_REG, 4, 5, 24);
#endif

    sr32(PMC_BASE+0x100,12,1,1);
    while(!(get_wvalue(PMC_BASE+0x104)&(0x1<<12)));

    //enable clk_ck1_hifi5
    //sr32(CCMU_AON_BASE+0xa4,  11, 1, 0x1);
    clk_ck1_hifi5 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK1_HIFI5);
    ret = hal_clock_enable(clk_ck1_hifi5);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err1;
    }

    //enable clk_ck3_hifi5
    //sr32(CCMU_AON_BASE+0xa8,  11, 1, 0x1);
    clk_ck3_hifi5 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CK3_HIFI5);
    ret = hal_clock_enable(clk_ck3_hifi5);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err2;
    }

    //set clk_ck1_hifi5 clk to 384M
    /* sr32(CCMU_AON_BASE+0xa4,  8, 3, 0x2); */

    //set clk_ck3_hifi5 clk to 400M
    //sr32(CCMU_AON_BASE+0xa8,  8, 3, 0x3);
    ret = hal_clk_set_rate(clk_ck3_hifi5, DSP_CORE_CLOCK_FREQ);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err2;
    }

    /*set clk_ck_hifi5 source to clk_ck3_hifi5*/
    //sr32(CCMU_AON_BASE+0xe0, 18, 1, 0x1);
    clk_ck_hifi5 = hal_clock_get(HAL_SUNXI_AON_CCU, CLK_CKPLL_HIFI5_SEL);
    ret = hal_clk_set_parent(clk_ck_hifi5, clk_ck3_hifi5);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err3;
    }

    //set clk_ck_hifi5_div source to clk_ck_hifi5
    //sr32(CCMU_BASE+0x068, 4, 2, 0x2);
    clk_hifi5_mux = hal_clock_get(HAL_SUNXI_CCU, CLK_DSP_SEL);
    ret = hal_clk_set_parent(clk_hifi5_mux, clk_ck_hifi5);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err4;
    }

    //set dsp_clk_hifi5_div to clk_ck_hifi5/1
    //sr32(CCMU_BASE+0x068, 0, 2, 0x0);
    clk_hifi5_div = hal_clock_get(HAL_SUNXI_CCU, CLK_DSP_DIV);
    ret = hal_clk_set_rate(clk_hifi5_div, DSP_CORE_CLOCK_FREQ);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err5;
    }

    //sr32(CCMU_BASE+0x068,31,1,1); //hifi5 core clk enable
	//dsp core clock gate
    clk_hifi5_core = hal_clock_get(HAL_SUNXI_CCU, CLK_DSP_GATE);
    ret = hal_clock_enable(clk_hifi5_core);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err6;
    }

    //sr32(CCMU_BASE+0x014,11,1,1); //hifi5 cfg clk gating
    clk_hifi5_cfg = hal_clock_get(HAL_SUNXI_CCU, CLK_DSP_CFG);
    ret = hal_clock_enable(clk_hifi5_cfg);
    if (HAL_CLK_STATUS_OK != ret) {
        ret = -1;
        goto err7;
    }

    //sr32(CCMU_BASE+0x018,11,1,1); //hifi5 cfg rst
    rst_hifi5_cfg = hal_reset_control_get(HAL_SUNXI_RESET, RST_DSP_CFG);
    if (hal_reset_control_deassert(rst_hifi5_cfg)) {
        ret = -1;
        goto err8;
    }

    sr32(DSP0_CTRL_REG0,1,1,1);

    hal_udelay(1000);
    put_wvalue(DSP0_ALTRST_REG, dsp_start_addr);

    sr32(DSP0_CTRL_REG0,0,1,1);//stall

    //sr32(CCMU_DSP_RST_REG,14,1,1);//
    rst_hifi5_debug = hal_reset_control_get(HAL_SUNXI_RESET, RST_DSP_DEBUG);
    if (hal_reset_control_deassert(rst_hifi5_debug)) {
        ret = -1;
        goto err9;
    }

    //no dsp debug clock gate
    //sr32(CCMU_DSP_GAT_REG,14,1,1);//dbg

    //sr32(CCMU_DSP_RST_REG,8,1,1);
    rst_hifi5_core = hal_reset_control_get(HAL_SUNXI_RESET, RST_DSP_CORE);
    hal_reset_control_assert(rst_hifi5_core);
    hal_udelay(10000);
    if (hal_reset_control_deassert(rst_hifi5_core)) {
        ret = -1;
        goto err10;
    }

    //bit8 is not the control bit of dsp core clock gate
    //sr32(CCMU_DSP_GAT_REG,8,1,1);//core

    sr32(DSP0_CTRL_REG0,0,1,0);//run
err10:
    hal_reset_control_put(rst_hifi5_core);
err9:
    hal_reset_control_put(rst_hifi5_debug);
err8:
    hal_reset_control_put(rst_hifi5_cfg);
err7:
    hal_clock_put(clk_hifi5_cfg);
err6:
    hal_clock_put(clk_hifi5_core);
err5:
    hal_clock_put(clk_hifi5_div);
err4:
    hal_clock_put(clk_hifi5_mux);
err3:
    hal_clock_put(clk_ck_hifi5);
err2:
    hal_clock_put(clk_ck3_hifi5);
err1:
    hal_clock_put(clk_ck1_hifi5);
    return ret;
}

int sun20i_boot_dsp_with_start_addr(uint32_t dsp_start_addr)
{
	printf("---boot dsp(start_addr: 0x%08x)---\n", dsp_start_addr);
	return __sun20i_boot_dsp_with_start_addr(dsp_start_addr);
}

int sun20i_boot_dsp(void)
{
#if CONFIG_ARCH_DSP_START_ADDRESS
    return sun20i_boot_dsp_with_start_addr(CONFIG_ARCH_DSP_START_ADDRESS);
#else
    return sun20i_boot_dsp_with_start_addr(rtos_spare_head.reserved[1]);
#endif
}
#endif

void start_kernel(void)
{
    portBASE_TYPE ret;

    //set default cpu voltage, M33: 192M@1100mV, RV: 480M@1100mV
#ifdef CONFIG_DRIVERS_PRCM
    HAL_PRCM_SetAPPLDOWorkVoltage(1100);
#else
#define GPRCM_APP_LDO_CTRL_REG (0x40050000 + 0x44)
    sr32(GPRCM_APP_LDO_CTRL_REG, 9, 5, 20);
#endif

#ifdef CONFIG_ARM_MPU
    void init_mpu(void);
    init_mpu();
#endif

#ifdef CONFIG_ARCH_HAVE_ICACHE
    hal_icache_init();
#endif

#ifdef CONFIG_ARCH_HAVE_DCACHE
    hal_dcache_init();
#endif

#ifdef CONFIG_COMPONENTS_STACK_PROTECTOR
    void stack_protector_init(void);
    stack_protector_init();
#endif
    systeminit();

    /* Init heap */
    heap_init();

#ifdef CONFIG_COMPONENTS_AW_SYS_CONFIG_SCRIPT
    hal_cfg_init();
#endif

#ifdef CONFIG_COMPONENTS_PM
    pm_wakecnt_init();
    pm_wakesrc_init();
    pm_devops_init();
    pm_syscore_init();
#endif

    /* Init hardware devices */
    prvSetupHardware();

    setbuf(stdout, 0);
    setbuf(stdin, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

#ifdef CONFIG_COMPONENT_CPLUSPLUS
	/* It should be called after the stdout is ready, otherwise the std:cout can't work  */
    int cplusplus_system_init(void);
    cplusplus_system_init();
#endif

    printf("M33 CPU Clock Freq: %d MHz\n", cpu_rate / 1000 / 1000);

#ifdef CONFIG_ARM_MPU
    //printf("init mpu\n");
#endif

#ifdef CONFIG_ARCH_HAVE_ICACHE
    //printf("init icache\n");
#endif

#ifdef CONFIG_ARCH_HAVE_DCACHE
    //printf("init dcache\n");
#endif

#ifdef CONFIG_CHECK_ILLEGAL_FUNCTION_USAGE
    void __cyg_profile_func_init(void);
    __cyg_profile_func_init();
#endif

    void cpu0_app_entry(void*);
    ret = xTaskCreate(cpu0_app_entry, (signed portCHAR *) "init-thread-0", 4096, NULL, 31, NULL);
    if (ret != pdPASS)
    {
        printf("Error creating task, status was %d\n", ret);
    }

    vTaskStartScheduler();
}


