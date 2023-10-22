// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2017, STMicroelectronics - All Rights Reserved
 * Author(s): Vikas Manocha, <vikas.manocha@st.com> for STMicroelectronics.
 */

#include <common.h>
#include <errno.h>
#include <asm/armv8m.h>
#include <asm/io.h>

void dcache_enable(void)
{
	return;
}

void dcache_disable(void)
{
	return;
}

int dcache_status(void)
{
	return 0;
}

void flush_dcache_all(void)
{
}

void invalidate_dcache_all(void)
{
}
void icache_enable(void)
{
	return;
}

void icache_disable(void)
{
	return;
}

int icache_status(void)
{
	return 0;
}

#ifdef CONFIG_SUNXI
void enable_caches(void)
{
#if !CONFIG_IS_ENABLED(SYS_ICACHE_OFF)
	icache_enable();
#endif
#if !CONFIG_IS_ENABLED(SYS_DCACHE_OFF)
	dcache_enable();
#endif
}
#endif
