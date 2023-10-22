/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
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

/*do not open source*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sunxi_hal_common.h>

#include "aw_breakpoint.h"
#include "cm33_wbp.h"

#define DWT_ARMV8M

#define DWT_CTRL       0xE0001000
#define DWT_CYCCNT     0xE0001004
#define DWT_COMP0      0xE0001020
#define DWT_MASK0      0xE0001024
#define DWT_FUNCTION0  0xE0001028
#define DWT_DEMCR      0xE000EDFC

/* DCB_DEMCR bit and field definitions */
#define TRCENA         (1 << 24)
#define MON_EN         (1 << 16)

#define DWT_LOG_ERR(fmt, ...)   printf("<E>DWT %s[%d]: "fmt"\r\n", __func__, __LINE__, ##__VA_ARGS__)

struct cortex_m_dwt_comp_t {
	unsigned int used;
	unsigned int comp;
	unsigned int mask;
	unsigned int function;
	unsigned int dwt_comp_address;
    unsigned int id;
};

struct cortex_m_dwt_t {
	unsigned int dwt_num_comp;
	unsigned int dwt_comp_available;
	struct cortex_m_dwt_comp_t *dwt_comparator_list;
};

static struct cortex_m_dwt_t g_dwt;

static int add_check(struct watchpoint *watchpoint)
{
	unsigned int dwt_num = 0, mask;

	struct cortex_m_dwt_comp_t *comparator;

	/*  check mask: hardware allows address masks of up to 32K */
	for (mask = 0; mask < 16; mask++) {
		if ((1u << mask) == watchpoint->length)
			break;
	}
	if (mask == 16) {
		DWT_LOG_ERR("unsupported watchpoint length : %d", watchpoint->length);
		return -1;
	}
	if (watchpoint->address & ((1 << mask) - 1)) {
		DWT_LOG_ERR("watchpoint address is unaligned : 0x%08x", watchpoint->address);
		return -1;
	}

	/*  check same watchpoint*/
	for (comparator = g_dwt.dwt_comparator_list;
		 comparator->comp == watchpoint->address && dwt_num < g_dwt.dwt_num_comp;
		 comparator++, dwt_num++) {
		DWT_LOG_ERR("find same watchpoint address, watchpoint address=0x%08x", watchpoint->address);
		return -1;
	}

	return 0;
}

static int cortex_m_dwt_add(struct watchpoint *watchpoint)
{
	int ret;
	unsigned int dwt_num = 0;

	struct cortex_m_dwt_comp_t *comparator;

	if (g_dwt.dwt_comp_available == 0) {
		DWT_LOG_ERR("no available comparators. only %u comparators", g_dwt.dwt_num_comp);
		return -1;
	}

	ret = add_check(watchpoint);
	if (ret) {
		return -1;
	}

	/* find free comparator*/
	for (comparator = g_dwt.dwt_comparator_list;
		 comparator->used && dwt_num < g_dwt.dwt_num_comp;
		 comparator++, dwt_num++)
		continue;

	if (dwt_num >= g_dwt.dwt_num_comp) {
		DWT_LOG_ERR("Can not find free DWT Comparator");
		return -1;
	}
	comparator->used = 1;
	comparator->id = watchpoint->id;

	/*set dwt regs*/
	comparator->comp = watchpoint->address;
	writel(comparator->comp, comparator->dwt_comp_address + 0);

#ifdef DWT_ARMV8M
	unsigned int data_size = watchpoint->length >> 1;
	comparator->mask = (watchpoint->length >> 1) | 1;

	switch (watchpoint->rw) {
	case DWT_READ:
		comparator->function = 6;
		break;
	case DWT_WRITE:
		comparator->function = 5;
		break;
	case DWT_ACCESS:
		comparator->function = 4;
		break;
	case DWT_BREAKPOINT:
		comparator->function = 2 | (0x1e << 27);
		comparator->function |= (0x1e << 27); /* ID */
		break;
	default:
		break;
	}
	comparator->function = comparator->function | (1 << 4) | (data_size << 10);
#else
	unsigned int mask, temp;

	mask = 0;
	temp = watchpoint->length;
	while (temp) {
		temp >>= 1;
		mask++;
	}
	mask--;
	comparator->mask = mask;
	writel(comparator->mask, comparator->dwt_comp_address + 4);

	switch (watchpoint->rw) {
	case DWT_READ:
		comparator->function = 5;
		break;
	case DWT_WRITE:
		comparator->function = 6;
		break;
	case DWT_ACCESS:
		comparator->function = 7;
		break;
	default:
		break;
	}
#endif
	writel(comparator->function, comparator->dwt_comp_address + 8);

	g_dwt.dwt_comp_available--;

	return 0;
}


static int cortex_m_dwt_remove(struct watchpoint *watchpoint)
{
	unsigned int dwt_num = 0;

	struct cortex_m_dwt_comp_t *comparator;

	/*find comparator by address*/
	for (comparator = g_dwt.dwt_comparator_list;
		comparator->comp != watchpoint->address && dwt_num < g_dwt.dwt_num_comp;
		comparator++, dwt_num++)
		continue;

	for (comparator = g_dwt.dwt_comparator_list;
		comparator->id != watchpoint->id && dwt_num < g_dwt.dwt_num_comp;
		comparator++, dwt_num++)
		continue;

	if (dwt_num >= g_dwt.dwt_num_comp) {
		DWT_LOG_ERR("Invalid DWT Comparator number in watchpoint");
		return -1;
	}

	if (comparator->used == 0) {
		DWT_LOG_ERR("Invalid DWT Comparator status in watchpoint");
		return -1;
	}

	/*unset comparator*/
	comparator->used     = 0;
	comparator->comp     = 0;
	comparator->function = 0;
	comparator->id       = 0;
	writel(comparator->function, comparator->dwt_comp_address + 8);

	g_dwt.dwt_comp_available++;

	return 0;
}

static int cortex_m_dwt_setup(struct cortex_m_dwt_t *dwt)
{
	unsigned int dwtcr;
	unsigned int dwt_num = 0;

	struct cortex_m_dwt_comp_t *comparator;

	dwtcr = readl(DWT_CTRL);
	if (!dwtcr) {
		DWT_LOG_ERR("no DWT");
		return -1;
	}

	dwt->dwt_num_comp = (dwtcr >> 28) & 0xF;
	dwt->dwt_comp_available = dwt->dwt_num_comp;
	dwt->dwt_comparator_list = calloc(dwt->dwt_num_comp, sizeof(struct cortex_m_dwt_comp_t));
	if (!dwt->dwt_comparator_list) {
		dwt->dwt_num_comp = 0;
		DWT_LOG_ERR("out of mem");
		return -1;
	}

	/* set reg addr for each comp*/
	for (comparator = dwt->dwt_comparator_list;
		dwt_num < dwt->dwt_num_comp;
		comparator++, dwt_num++) {
		comparator->dwt_comp_address = DWT_COMP0 + dwt_num * 0x10;
		writel(0, comparator->dwt_comp_address + 8);
	}

	/* enable dwt and dwt exception*/
	writel(readl(DWT_DEMCR)| TRCENA | MON_EN, DWT_DEMCR);
	return 0;
}

static int cortex_m_dwt_free(struct cortex_m_dwt_t *dwt)
{
	free(dwt->dwt_comparator_list);
	dwt->dwt_num_comp = 0;
	dwt->dwt_comp_available = 0;
	return 0;
}

static int watchpoint_init(void)
{
	return cortex_m_dwt_setup(&g_dwt);
}

static int watchpoint_deinit(void)
{
	return cortex_m_dwt_free(&g_dwt);
}

static int watchpoint_add(struct watchpoint *watchpoint)
{
	return cortex_m_dwt_add(watchpoint);
}

static int watchpoint_del(struct watchpoint *watchpoint)
{
	return cortex_m_dwt_remove(watchpoint);
}

static int enable = 0;

int monitor_mode_enabled(void)
{
    if (readl(DWT_DEMCR) & MON_EN) {
        return 1;
    }
    return 0;
}

int enable_monitor_mode(void)
{
    return watchpoint_init();
}

int get_num_brp_resources(void)
{
	unsigned int dwtcr;
	int num = 0;

	dwtcr = readl(DWT_CTRL);
	if (!dwtcr) {
		DWT_LOG_ERR("no DWT");
		return 0;
	}

	num = ((dwtcr >> 28) & 0xF) - 1;
	if (num > 1) {
		return 1;
	}
	return 0;
}

int get_num_wrp_resources(void)
{
	unsigned int dwtcr;

	dwtcr = readl(DWT_CTRL);
	if (!dwtcr) {
		DWT_LOG_ERR("no DWT");
		return 0;
	}

	return ((dwtcr >> 28) & 0xF) - 1;
}

int arm_install_hw_breakpoint(int i, unsigned long addr)
{
	struct watchpoint wbp = {0};

	wbp.rw = DWT_BREAKPOINT;
	wbp.address = addr;

	if ((addr % 4) == 0) {
		wbp.length = 4;
	} else if((addr % 2) == 0) {
		wbp.length = 2;
	} else {
		wbp.length = 1;
	}
	wbp.id = i + (get_num_wrp_resources() - 1);

	return watchpoint_add(&wbp);
}

int arm_install_hw_watchpoint(enum gdb_bptype type, int i, unsigned long addr)
{
    struct watchpoint wbp = {0};

	if (type == BP_WRITE_WATCHPOINT) {
		wbp.rw = DWT_WRITE;
	} else if (type == BP_READ_WATCHPOINT) {
		wbp.rw = DWT_READ;
	} else if (type == BP_ACCESS_WATCHPOINT) {
		wbp.rw = DWT_ACCESS;
	}

	wbp.address = addr;
	wbp.length = 4;
	wbp.id = i;

	return watchpoint_add(&wbp);
}

void arm_uninstall_hw_watchpoint(int i)
{
    struct watchpoint wbp = {0};
	wbp.id = i;
	cortex_m_dwt_remove(&wbp);
}

void arm_uninstall_hw_breakpoint(int i)
{
	struct watchpoint wbp = {0};
	wbp.id = i + (get_num_wrp_resources() - 1);
	cortex_m_dwt_remove(&wbp);
}
