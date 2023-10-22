#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <platform.h>
#include <aw_io.h>
#include <console.h>

#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include <xtensa/tie/xt_hifi2.h>
#include <xtensa/config/core.h>
#include <xtensa/tie/xt_timer.h>

#ifdef CONFIG_DRIVERS_TIMER
#include <sunxi_hal_timer.h>
#endif

#define MEM_TYPE_DDR        (0)
#define MEM_TYPE_SRAM       (1)
#define MEM_TYPE_LOCALRAM   (2)
#define MEM_TYPE_LPSRAM     (3)
#define MEM_TYPE_HPSRAM     (4)
#define MEM_TYPE_MAX        (5)

#define PERF_MEM_USAGE \
	"perf_mem usage:\n" \
	"param 1: test memory type:\n" \
		"\t0->ddr 1->sram 2->localram 3->lpsram 4->hpsram\n" \
	"param 2: test times\n" \
	"param 3: remap bit value:\n" \
		"\t0->access using external bus\n" \
		"\t1->access using internal bus\n" \

struct memory_msg_t{
	uint32_t addr_start;
	uint32_t len;
};

#ifdef CONFIG_ARCH_SUN20IW2

static struct memory_msg_t memory_msg[]={
	[MEM_TYPE_DDR]  = {
			.addr_start = 0,
			.len = 0,
			},
	[MEM_TYPE_SRAM] = {
			.addr_start = 0x040c0000,
			.len = 1024*256,
			},
	[MEM_TYPE_LOCALRAM]  = {
			.addr_start = 0,
			.len = 0,
			},
	[MEM_TYPE_LPSRAM] = {
			.addr_start = 0x08000000,
			.len = 1024*2048,
			},
	[MEM_TYPE_HPSRAM] = {
			.addr_start = 0x0c000000,
			.len = 1024*2048,
			},
};

#ifdef CONFIG_DRIVERS_TIMER

#define _PERF_TIMER_CNTVAL_REG(val) (SUNXI_TMR_PBASE + 0x10 * (val) + 0x18)

static void hstimer_init(void)
{
	hal_timer_init(SUNXI_TMR0);
	hal_timer_set_oneshot(SUNXI_TMR0, 1000*1000*60*30, NULL, NULL);
	vTaskDelay(1);
}

static void hstimer_deinit(void)
{
	hal_timer_stop(SUNXI_TMR0);
	hal_timer_uninit(SUNXI_TMR0);
}

static uint32_t hstimer_value_read(void)
{
	uint32_t t = readl((unsigned int)_PERF_TIMER_CNTVAL_REG(SUNXI_TMR0));
	return t;
}

static uint32_t htimer_get_freq(void)
{
	return 40000000;
}

static void remap_bit_set(int value)
{
}
#endif

#else
static struct memory_msg_t memory_msg[]={
	[MEM_TYPE_DDR] = {
			.addr_start = 0x32080000,
			.len = 1024*512,
			},
	[MEM_TYPE_SRAM] = {
			.addr_start = 0x20020000,
			.len = 1024*32,
			},
	[MEM_TYPE_LOCALRAM] = {
			.addr_start = 0x20028000,
			.len = 1024*128,
			},
	[MEM_TYPE_LPSRAM] = {
			.addr_start = 0,
			.len = 0,
			},
	[MEM_TYPE_HPSRAM] = {
			.addr_start = 0,
			.len = 0,
			},
};

static void hstimer_init(void)
{
	//hstimer clk
	writel(0x1,0x0200173c);
	writel(0x10001,0x0200173c);
	//hstimer config
	writel(0xffffffff,0x03008024);
	writel(0x80,0x03008020);
	writel(0x82,0x03008020);
	writel(0x83,0x03008020);
}

static void hstimer_deinit(void)
{

}

static uint32_t hstimer_value_read(void)
{
    return (uint32_t)readl(0x0300802c);
}

static uint32_t htimer_get_freq(void)
{
	return 200000000;
}

static void remap_bit_set(int value)
{
	int val = 0;

	val = readl(0x03000008);
	val &= ~(1 << 0);
	writel(val,0x03000008);
	val = readl(0x03000008);
	val |= (value << 0);
	writel(val,0x03000008);
}
#endif

static void perf_mem_usage()
{
	printf("%s", PERF_MEM_USAGE);
}

ae_int64 memoryDWordRead(uint32_t startAddres, uint32_t len)
{
	ae_int64 data1 = 0;
	ae_int64 *paddress;
	ae_int64 *enddress;
	paddress = (ae_int64 *)startAddres;
	enddress = (ae_int64 *)(startAddres + len);
	for(; (ae_int64 *)paddress < (ae_int64 *)enddress; (ae_int64 *)paddress++)
		data1 = *paddress;
	return data1;
}

ae_int64 memoryDWordWrite(uint32_t startAddres, uint32_t len)
{
	ae_int64 data = 0x12345678;
	ae_int64 *paddress;
	ae_int64 *enddress;
	paddress = (ae_int64 *)startAddres;
	enddress = (ae_int64 *)(startAddres + len);
	for(; (ae_int64 *)paddress < (ae_int64 *)enddress; (ae_int64 *)paddress++)
		*paddress = data;
	return data;
}

int cmd_perf_mem( int argc, char **argv )
{
	ae_int64 data = 0;
	uint32_t wantbytes = 0;
	int times;
	int memory_tpye;
	int remap_bit_value;
	int i_times;
	int time_cnt = 0;
	char mode = 'r';
	uint32_t t_before, t_after;

	if (argc > 5) {
		perf_mem_usage();
		return -1;
	}

	sscanf(argv[1], "%d", &memory_tpye);
	if(memory_tpye >= MEM_TYPE_MAX){
		printf("memory tpye err!\n");
		perf_mem_usage();
		return -1;
	}
	wantbytes = memory_msg[memory_tpye].len;

	sscanf(argv[2], "%d", &times);
	sscanf(argv[3], "%d", &remap_bit_value);
	if (argc > 4)
		sscanf(argv[4], "%c", &mode);

	switch (mode) {
	case 'r':
		printf("Test r mode\n");
		break;
	case 'w':
		printf("Test w mode\n");
		break;
	default:
		printf("Error mode, please input again\n");
		perf_mem_usage();
		return -1;
	}

	if (remap_bit_value > 1) {
		printf("remap bit err!\n");
		perf_mem_usage();
		return -1;
	}
	remap_bit_set(remap_bit_value);

	printf("got %0.5fMB (%u bytes)\n"
		"times: %d (total test %u bytes)\n", wantbytes*1.0/1024/1024,
						wantbytes, times,
						wantbytes * times);

	uint32_t addr_start = memory_msg[memory_tpye].addr_start;
	uint32_t len = memory_msg[memory_tpye].len;
	printf("test addr start : 0x%x, size : 0x%x \n",addr_start,len);

	//clean cache
	xthal_icache_all_invalidate();
	xthal_dcache_all_writeback_inv();

	//init hstimer again
	hstimer_init();
	t_before = hstimer_value_read();

	if (mode == 'r') {
	//test memory read
		for (i_times = 0; i_times < times; i_times++)
			data = memoryDWordRead(addr_start,len);
	} else {
		for (i_times = 0; i_times < times; i_times++)
			data = memoryDWordWrite(addr_start,len);
	}

	t_after = hstimer_value_read();
	time_cnt = t_before - t_after;
	if (time_cnt == 0) {
		printf("time_cnt inf\n");
	}else {
		float hstimer_ = (1.0 * time_cnt) / htimer_get_freq();
		printf("time passed: %0.8f(s)\n", hstimer_);
		printf("----> %0.1fMB/s \n",(1.0*times*wantbytes/1024/1024)/hstimer_);
	}
	hstimer_deinit();
	return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_perf_mem, perf_mem, Memory access performance test);
