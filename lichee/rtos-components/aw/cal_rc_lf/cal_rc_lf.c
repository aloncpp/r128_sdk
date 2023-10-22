#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <hal_cmd.h>
#include <osal/hal_time.h>
#include <sunxi_hal_rcosc_cali.h>

//#define CAL_RC_LF_DEBUG

#include <timer/platform_timer.h>

#define TIMER_BASE_ADDR SUNXI_TMR_PBASE

#define TIMER_IRQ_CTRL_OFFSET 0x00
#define TIMER_IRQ_STATUS_OFFSET 0x04

//bit7, mode, bit6:4, pre_div, bit3-bit2, clk source, bit1, reload, bit0, enable
#define REF_TIMER_CTRL_OFFSET 0x10
#define REF_TIMER_INTERVAL_OFFSET 0x14
#define REF_TIMER_CNT_OFFSET 0x18

#define TARGET_TIMER_CTRL_OFFSET 0x20
#define TARGET_TIMER_INTERVAL_OFFSET 0x24
#define TARGET_TIMER_CNT_OFFSET 0x28

#define TIMER_RELOAD_VALUE 0xFFFFFFFF

#define REF_TIMER_ID 0
#define TARGET_TIMER_ID 1

static uint32_t s_ref_timer_clk_freq;

static void dump_timer_reg(void)
{
	printf("Timer module related registers:\n");
	printf("IRQ_CTRL: 0x%08x\n", readl(TIMER_BASE_ADDR + TIMER_IRQ_CTRL_OFFSET));
	printf("IRQ_STATUS: 0x%08x\n", readl(TIMER_BASE_ADDR + TIMER_IRQ_STATUS_OFFSET));
	printf("REF_TIMER_CTRL: 0x%08x\n", readl(TIMER_BASE_ADDR + REF_TIMER_CTRL_OFFSET));
	printf("REF_TIMER_INTERVAL: 0x%08x\n", readl(TIMER_BASE_ADDR + REF_TIMER_INTERVAL_OFFSET));
	printf("REF_TIMER_CNT: 0x%08x\n", readl(TIMER_BASE_ADDR + REF_TIMER_CNT_OFFSET));
	printf("TARGET_TIMER_CTRL: 0x%08x\n", readl(TIMER_BASE_ADDR + TARGET_TIMER_CTRL_OFFSET));
	printf("TARGET_TIMER_INTERVAL: 0x%08x\n", readl(TIMER_BASE_ADDR + TARGET_TIMER_INTERVAL_OFFSET));
	printf("TARGET_TIMER_CNT: 0x%08x\n", readl(TIMER_BASE_ADDR + TARGET_TIMER_CNT_OFFSET));
}

static void init_ref_timer(void)
{
	uint32_t reg_data, reg_addr;
	//disable int
	reg_addr = TIMER_BASE_ADDR + TIMER_IRQ_CTRL_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~(1 << REF_TIMER_ID);
	writel(reg_data, reg_addr);

	//disable timer
	reg_addr = TIMER_BASE_ADDR + REF_TIMER_CTRL_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~(1 << 0);
	writel(reg_data, reg_addr);

	//set interval
	reg_addr = TIMER_BASE_ADDR + REF_TIMER_INTERVAL_OFFSET;
	reg_data = TIMER_RELOAD_VALUE;
	writel(reg_data, reg_addr);

	//single mode, HOSC clock, 128 div
	reg_addr = TIMER_BASE_ADDR + REF_TIMER_CTRL_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~((1 << 7) | (0x7 << 4) | (0x3 << 2));
	reg_data |= (1 << 7) | (0x7 << 4) | (1 << 2);
	writel(reg_data, reg_addr);

	s_ref_timer_clk_freq = HAL_GetHFClock() / 128;
#ifdef CAL_RC_LF_DEBUG
	printf("ref timer input clock(HOSC): %uHz\n", s_ref_timer_clk_freq);
#endif
}

static void init_target_timer_with_interval_value(uint32_t interval_value)
{
	uint32_t reg_data, reg_addr;
	//disable int
	reg_addr = TIMER_BASE_ADDR + TIMER_IRQ_CTRL_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~(1 << TARGET_TIMER_ID);
	writel(reg_data, reg_addr);

	//disable timer
	reg_addr = TIMER_BASE_ADDR + TARGET_TIMER_CTRL_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~(1 << 0);
	writel(reg_data, reg_addr);

	//clear irq pending
	reg_addr = TIMER_BASE_ADDR + TIMER_IRQ_STATUS_OFFSET;
	reg_data = readl(reg_addr);
	reg_data |= (1 << TARGET_TIMER_ID);
	writel(reg_data, reg_addr);

	//set interval
	reg_addr = TIMER_BASE_ADDR + TARGET_TIMER_INTERVAL_OFFSET;
	reg_data = interval_value;
	writel(reg_data, reg_addr);

	//single mode, SYS_32K clock, 1 div
	reg_addr = TIMER_BASE_ADDR + TARGET_TIMER_CTRL_OFFSET;
	reg_data = readl(reg_addr);
	reg_data &= ~((1 << 7) | (0x7 << 4) | (0x3 << 2));
	reg_data |= (1 << 7);
	writel(reg_data, reg_addr);

	//s_timer_clk_freq[1] = 32000;
	//printf("target timer input clock(SYS_32K): %uHz\n", s_timer_clk_freq[1]);
}

static void init_target_timer(void)
{
	init_target_timer_with_interval_value(TIMER_RELOAD_VALUE);
}

static inline void start_timer(uint32_t ctrl_reg_offset)
{
	uint32_t reg_data, reg_addr;
	//reload and enable
	reg_addr = TIMER_BASE_ADDR + ctrl_reg_offset;
	reg_data = readl(reg_addr);
	reg_data |= (1 << 1) | (1 << 0);
	writel(reg_data, reg_addr);
}

static inline void start_ref_timer(void)
{
	start_timer(REF_TIMER_CTRL_OFFSET);
}

static inline void start_target_timer(void)
{
	start_timer(TARGET_TIMER_CTRL_OFFSET);
}

static inline void stop_timer(uint32_t ctrl_reg_offset)
{
	uint32_t reg_data, reg_addr;
	//disable timer
	reg_addr = TIMER_BASE_ADDR + ctrl_reg_offset;
	reg_data = readl(reg_addr);
	reg_data &= ~(1 << 0);
	writel(reg_data, reg_addr);
}

static inline void stop_ref_timer(void)
{
	stop_timer(REF_TIMER_CTRL_OFFSET);
}

static inline void stop_target_timer(void)
{
	stop_timer(TARGET_TIMER_CTRL_OFFSET);
}

static inline uint32_t read_timer(uint32_t cnt_reg_offset)
{
	return TIMER_RELOAD_VALUE - readl(TIMER_BASE_ADDR + cnt_reg_offset);
}

static int is_target_timer_irq_pending(void)
{
	uint32_t reg_data, reg_addr;
	//disable timer
	reg_addr = TIMER_BASE_ADDR + TIMER_IRQ_STATUS_OFFSET;
	reg_data = readl(reg_addr);

	if (reg_data & (1 << TARGET_TIMER_ID))
	{
		return 1;
	}
	return 0;
}

/*
使用输入时钟为高频晶振时钟的定时器来校准输入时钟频率为RC_LF/32的定时器的定时值
*/
#define MEASURE_TIME_MS 100
#define MAX_TRY_READ_TIMER_TIMES 200

int get_corrected_counter_value_for_rc_lf(uint32_t target_value, uint32_t *corrected_value)
{
	uint32_t ref_timer_cnt = 0, target_timer_cnt = 0;
	uint32_t loop_cnt = 0, last_timer_cnt = 0, timer_cnt = 0;

	init_ref_timer();
	init_target_timer();

	ref_timer_cnt = MEASURE_TIME_MS * s_ref_timer_clk_freq / 1000;
#ifdef CAL_RC_LF_DEBUG
	printf("ref_timer_cnt: %u\n", ref_timer_cnt);
	//dump_timer_reg();
#endif

	last_timer_cnt = read_timer(REF_TIMER_CNT_OFFSET);
	start_ref_timer();
	start_target_timer();

	while (1)
	{
		timer_cnt = read_timer(REF_TIMER_CNT_OFFSET);
		if (last_timer_cnt != timer_cnt)
		{
			break;
		}
		last_timer_cnt = timer_cnt;
		
		loop_cnt++;
		if (loop_cnt >= MAX_TRY_READ_TIMER_TIMES)
		{
			return -1;
		}
	}
	//dump_timer_reg();

	while (1)
	{
		timer_cnt = read_timer(REF_TIMER_CNT_OFFSET);
		if (timer_cnt >= ref_timer_cnt)
		{
			//dump_timer_reg();
			stop_ref_timer();
			stop_target_timer();
			target_timer_cnt = read_timer(TARGET_TIMER_CNT_OFFSET);
			break;
		}
	}

#ifdef CAL_RC_LF_DEBUG
	printf("loop_cnt: %u\n", loop_cnt);
	printf("target_timer_cnt: %u\n", target_timer_cnt);
#endif

	uint64_t tmp;
	tmp = (uint64_t)target_value * target_timer_cnt / 32 / MEASURE_TIME_MS;

	if (tmp > UINT32_MAX)
	{
		return -2;
	}

	*corrected_value = tmp;
    return 0;
}

/* the unit of input parameter: ms */
int get_corrected_time_for_rc_lf(uint32_t target_time, uint32_t *corrected_time)
{
	uint32_t max_time = UINT32_MAX / 32;
	if (target_time > max_time)
	{
		target_time = max_time;
	}

	uint32_t corrected_value = 0;
	int ret = get_corrected_counter_value_for_rc_lf(target_time * 32, &corrected_value);
	if (!ret)
	{
		*corrected_time = corrected_value / 32;
	}
	return ret;
}


#ifdef CONFIG_COMPONENTS_CAL_RC_LF_TEST
int cmd_test_cal(int argc, char **argv)
{
	int ret = 0;
	uint32_t rc_lf_freq = 0;
	hal_status_t ret2 = hal_get_rc_lf_freq(&rc_lf_freq);
	if (ret2 != HAL_OK)
	{
		printf("hal_get_rc_lf_freq failed, ret: %d\n", ret2);
		return -1;
	}
	printf("RC_LF frequency: %uHz\n", rc_lf_freq);

	uint32_t target_value = 32000, corrected_value = 0;
	ret = get_corrected_counter_value_for_rc_lf(target_value, &corrected_value);
	if (ret)
	{
		printf("get_corrected_counter_value_for_rc_lf failed, ret: %d\n", ret);
		dump_timer_reg();
	}
	printf("target_value: %u, corrected_value: %u\n", target_value, corrected_value);

	uint32_t target_time = 1500, corrected_time = 0;
	ret = get_corrected_time_for_rc_lf(target_time, &corrected_time);
	if (ret)
	{
		printf("get_accurate_time_for_rc_lf failed, ret: %d\n", ret);
	}
	printf("target_time: %ums, corrected_time: %ums\n", target_time, corrected_time);
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_cal, test_cal_rc_lf, calibrate the counter value of timer whose input clock is RC_LF/32)


#include <errno.h>

static void print_cal_rc_lf_cmd_usage(void)
{
	printf("Usage: test_cal_rc_lf2 <target_value>\n");
	printf("       the unit of parameter 'target_value/32000' is second\n");
}

int cmd_test_cal2(int argc, char **argv)
{
	char *ptr = NULL;
	errno = 0;

	if (argc != 2)
	{
		printf("invalid input parameter num(%d)!\n", argc);
		print_cal_rc_lf_cmd_usage();
		return 0;
	}
	uint32_t target_value = strtoul(argv[1], &ptr, 10);
	if (errno || (ptr && *ptr != '\0'))
	{
		printf("invalid input parameter('%s')!\n", argv[1]);
		print_cal_rc_lf_cmd_usage();
		return 0;
	}

	printf("target_value: %u\n", target_value);

	int ret = 0;
	uint32_t rc_lf_freq = 0;
	hal_status_t ret2 = hal_get_rc_lf_freq(&rc_lf_freq);
	if (ret2 != HAL_OK)
	{
		printf("hal_get_rc_lf_freq failed, ret: %d\n", ret2);
		return -1;
	}
	printf("RC_LF frequency: %uHz\n", rc_lf_freq);

	uint32_t corrected_value = 0;
	ret = get_corrected_counter_value_for_rc_lf(target_value, &corrected_value);
	if (ret)
	{
		printf("get_corrected_counter_value_for_rc_lf failed, ret: %d\n", ret);
		return -2;
	}
	printf("corrected_value: %u\n", corrected_value);

	init_target_timer_with_interval_value(corrected_value);
	printf("start timer\n");
	start_target_timer();
	while (1)
	{
		if (is_target_timer_irq_pending())
		{
			printf("target_value %u reach\n", target_value);
			break;
		}
	}
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_cal2, test_cal_rc_lf2, test the corrected value of timer)

#endif
