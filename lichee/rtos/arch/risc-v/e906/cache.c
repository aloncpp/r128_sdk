/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the People's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
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

#include <csr.h>

#define L1_CACHE_BYTES (32)

static void dcache_wb_range(unsigned long start, unsigned long end)
{
    register unsigned long i asm("a5");
    i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile(".word 0x0297800b\n":::"memory");
    }
    asm volatile(".word 0x0000100f":::"memory");
}

static void dcache_inv_range(unsigned long start, unsigned long end)
{
    register unsigned long i asm("a5");
    i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile(".word 0x02a7800b\n":::"memory");
    }
    asm volatile(".word 0x0000100f":::"memory");
}

static void dcache_wbinv_range(unsigned long start, unsigned long end)
{
    register unsigned long i asm("a5");
    i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile(".word 0x02b7800b\n":::"memory");
    }
    asm volatile(".word 0x0000100f":::"memory");
}

static void icache_inv_range(unsigned long start, unsigned long end)
{
    unsigned long i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile(".word 0x0387800b\n":::"memory");
    }
    asm volatile(".word 0x0000100f":::"memory");
}

void FlushDcacheAll(void)
{
    /* dcache.call */
    asm volatile(".word 0x0010000b":::"memory");
}

void awos_arch_clean_flush_dcache(void)
{
    /* dcache.ciall */
    asm volatile(".word 0x0030000b":::"memory");
}

void InvalidDcache(void)
{
    /* dcache.iall */
    asm volatile(".word 0x0020000b":::"memory");
}

void FlushIcacheAll(void)
{
    /* icache.iall */
    asm volatile(".word 0x0100000b":::"memory");
}

void InvalidIcacheRegion(unsigned long start, unsigned int len)
{
    icache_inv_range(start, start + len);
}

void awos_arch_mems_flush_icache_region(unsigned long start, unsigned long len)
{
    icache_inv_range(start, start + len);
}

void FlushDcacheRegion(unsigned long start, unsigned long len)
{
    dcache_wb_range(start, start + len);
}

void awos_arch_mems_clean_flush_dcache_region(unsigned long start, unsigned long len)
{
    dcache_wbinv_range(start, start + len);
}

void InvalidDcacheRegion(unsigned long start, unsigned long len)
{
    dcache_inv_range(start, start + len);
}

void awos_arch_clean_flush_cache(void)
{
    InvalidDcache();
    FlushIcacheAll();
}

void awos_arch_clean_flush_cache_region(unsigned long start, unsigned long len)
{
    awos_arch_mems_clean_flush_dcache_region(start, len);
    awos_arch_mems_flush_icache_region(start, len);
}

void awos_arch_flush_cache(void)
{
	InvalidDcache();
	FlushIcacheAll();
}

int check_virtual_address(unsigned long vaddr)
{
    return 1;
}

void hardware_config(void)
{
    /*
    (0) IE=1时     Icache打开
    (1) DE=1时     Dcache打开
    (2) WB=1时     数据高速缓存为write-back模式(write-back数据写到dcache,write-though 数据写到内存)
    (3) WA=1时     数据高速缓存为write allocate模式
    (4) RS=1时     返回栈开启
    (5) BPE=1时    预测跳转开启
    (12) BTB=1时   分支目标预测开启
    */
    csr_write(CSR_MHCR, (1 << 12) | (0xf << 2));

    /*
    (10) PMDU=0 用户模式下事件检测计数器正常计数, =1禁止计数
    (13) PMDM=0 机器模式下事件检测计数器正常计数, =1禁止计数
    (15) MM为1时支持非对齐访问，硬件处理非对齐访问
    (22) THEADISAEE为1时可以使用THEAD扩展指令集
    (30,31) PM: 当前特权模式, b11机器模式, b00用户模式
    */
    csr_set(CSR_MXSTATUS, (1 << 22) | (1 << 15));

    /*
    (20) AEE=1 精确异常,会导致Load/Store指令阻塞流水线,性能下降
    */
    //csr_write(CSR_MHINT, (1 << 20));
}


void dcache_enable(void)
{
    csr_set(CSR_MHCR, 0x1 << 1);
}

void icache_enable(void)
{
    /*
    (0) IE=1时Icache打开
    */
    csr_set(CSR_MHCR, 0x1);
}

void hal_dcache_init(void)
{
    dcache_enable();
}

void hal_icache_init(void)
{
    icache_enable();
}

void data_sync_barrier(void)
{
    asm volatile(".word 0x0000100f":::"memory");
}
