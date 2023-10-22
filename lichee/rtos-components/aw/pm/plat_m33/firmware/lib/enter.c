#include "type.h"
#include "head.h"
#include "arch.h"
#include "driver_lpsram.h"
#include "driver_hpsram.h"
#include "systick.h"
#include "pm_wakeres.h"
#include "pm_testlevel.h"
#include "pm_base.h"

#include "delay.h"
#include "standby_clk.h"
#include "standby_power.h"
#include "cache.h"
#include "standby_scb.h"
#include <wakeup.h>

static inline void record_stage(uint32_t val)
{
	if (head->stage_record)
		writel(val, PM_STANDBY_STAGE_M33_REC_REG);
}

int standby_enter(void)
{
	record_stage(PM_TEST_RECORDING_ENTER | 0x10);
	/* save SCB regs */
	scb_save();

	/* clean & invalid dcache before psram enter retention
	 * to avoid to flush dada to psram later.
	 */
	standby_CleanInvalidateDCache();
	standby_DisableDCache();
	record_stage(PM_TEST_RECORDING_ENTER | 0x20);

	if (head->hpsram_inited && hpsram_is_running()) {
		if (head->hpsram_crc.crc_enable) {
			head->hpsram_crc.crc_before = hpsram_crc(head);
			hpsram_crc_save(head, HPSRAM_CRC_BEFORE);
		}
		record_stage(PM_TEST_RECORDING_ENTER | 0x30);
		hpsram_enter_retention(head);
		head->hpsram_inited = HPSRAM_SUSPENDED;
	}
	record_stage(PM_TEST_RECORDING_ENTER | 0x40);

	if (head->lpsram_inited && lpsram_is_running()) {
		if (head->lpsram_crc.crc_enable) {
			head->lpsram_crc.crc_before = lpsram_crc(head);
			lpsram_crc_save(head, LPSRAM_CRC_BEFORE);
		}
		record_stage(PM_TEST_RECORDING_ENTER | 0x50);
		lpsram_enter_retention(head);
		head->lpsram_inited = LPSRAM_SUSPENDED;
	}
	record_stage(PM_TEST_RECORDING_ENTER | 0x60);

	/* analog
	 * RTC GPIO: do not need ctrl
	 * AON GPIO: may have clk gating
	 */

	/* close systick. systick rely on DPLL clk */
	cpu_disable_systick();
	record_stage(PM_TEST_RECORDING_ENTER | 0x70);

	head->suspend_moment = pm_gettime_ns();

	/* clk */
	clk_suspend(head);
	record_stage(PM_TEST_RECORDING_ENTER | 0x80);

	/* power. controled by hardware */
	ldo_disable(head);
	record_stage(PM_TEST_RECORDING_ENTER | 0x90);


	if (head->time_to_wakeup_ms > 0)
		enable_wuptimer(head->time_to_wakeup_ms);
	record_stage(PM_TEST_RECORDING_ENTER | 0xa0);

	switch (head->mode)  {
		case PM_MODE_ON:
			break;
		case PM_MODE_SLEEP:
			_fw_cpu_sleep();
			break;
		case PM_MODE_STANDBY:
			writel(BOOT_FALG_DEEP_SLEEP, CPU_BOOT_FLAG_REG);
			_fw_cpu_standby();
			writel(BOOT_FALG_COLD_RESET, CPU_BOOT_FLAG_REG);
			break;
		case PM_MODE_HIBERNATION:
			hibernation_mode_set();
			writel(BOOT_FALG_COLD_RESET, CPU_BOOT_FLAG_REG);
			_fw_cpu_standby();
			/* cold reset without return */
			hibernation_mode_restore();
			break;
		default:
			break;
	}
	record_stage(PM_TEST_RECORDING_ENTER | 0xb0);


	ldo_enable(head);
	record_stage(PM_TEST_RECORDING_ENTER | 0xc0);

	clk_resume(head);
	record_stage(PM_TEST_RECORDING_ENTER | 0xd0);

	head->resume_moment = pm_gettime_ns();

	cpu_enable_systick();
	record_stage(PM_TEST_RECORDING_ENTER | 0xe0);

	if (head->lpsram_inited == LPSRAM_SUSPENDED) {
		lpsram_exit_retention(head);
		record_stage(PM_TEST_RECORDING_ENTER | 0xf0);
		if (head->lpsram_crc.crc_enable) {
			head->lpsram_crc.crc_after = lpsram_crc(head);
			lpsram_crc_save(head, LPSRAM_CRC_AFTER);
			if (head->lpsram_crc.crc_after != head->lpsram_crc.crc_before) {
                               writel(0xffffffff, 0x40050208);
                               while(1);
                       }
		}
		head->lpsram_inited = LPSRAM_RESUMED;
	}
	record_stage(PM_TEST_RECORDING_ENTER | 0xf1);

	if(head->hpsram_inited == HPSRAM_SUSPENDED) {
		hpsram_exit_retention(head);
		record_stage(PM_TEST_RECORDING_ENTER | 0xf2);
		if (head->hpsram_crc.crc_enable) {
			head->hpsram_crc.crc_after = hpsram_crc(head);
			hpsram_crc_save(head, HPSRAM_CRC_AFTER);
			if (head->hpsram_crc.crc_after != head->hpsram_crc.crc_before) {
                               writel(0xeeeeeeee, 0x40050208);
                               while(1);
                       }
		}
		head->hpsram_inited = HPSRAM_RESUMED;
	}
	record_stage(PM_TEST_RECORDING_ENTER | 0xf3);

	/* init cache, restore SCB regs can't restore cache */
	standby_EnableICache();
	standby_EnableDCache();

	/* restore M33 SCB */
	scb_restore();
	record_stage(PM_TEST_RECORDING_ENTER | 0xf4);

	return 0;
}

