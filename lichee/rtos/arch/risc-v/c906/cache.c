#include <csr.h>

#define L1_CACHE_BYTES (64)

static void dcache_wb_range(unsigned long start, unsigned long end)
{
    unsigned long i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile("dcache.cpa %0\n"::"r"(i):"memory");
    }
    asm volatile(".long 0x01b0000b");
}

static void dcache_inv_range(unsigned long start, unsigned long end)
{
    unsigned long i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile("dcache.ipa %0\n"::"r"(i):"memory");
    }
    asm volatile(".long 0x01b0000b");
}

static void dcache_wbinv_range(unsigned long start, unsigned long end)
{
    unsigned long i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile("dcache.cipa %0\n"::"r"(i):"memory");
    }
    asm volatile(".long 0x01b0000b");
}

static void icache_inv_range(unsigned long start, unsigned long end)
{
    unsigned long i = start & ~(L1_CACHE_BYTES - 1);

    for (; i < end; i += L1_CACHE_BYTES)
    {
        asm volatile("icache.ipa %0\n"::"r"(i):"memory");
    }
    asm volatile(".long 0x01b0000b");
}

void FlushDcacheAll(void)
{
    asm volatile("dcache.call\n":::"memory");
}

void awos_arch_clean_flush_dcache(void)
{
    asm volatile("dcache.ciall\n":::"memory");
}

void InvalidDcache(void)
{
    asm volatile("dcache.iall\n":::"memory");
}

void FlushIcacheAll(void)
{
    asm volatile("icache.iall\n":::"memory");
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

void cache_init(void)
{
	/*
	(0:1) CACHE_SEL=2’b11时，选中指令和数据高速缓存
	(4) INV=1时高速缓存进行无效化
	(16) BHT_INV=1时分支历史表内的数据进行无效化
	(17) TB_INV=1时分支目标缓冲器内的数据进行无效化
	*/
	csr_write(CSR_MCOR, 0x70013);

	/*
	(0) IE=1时Icache打开
	(1) DE=1时Dcache打开
	(2) WA=1时数据高速缓存为write allocate模式 (c906不支持)
	(3) WB=1时数据高速缓存为写回模式  (c906固定为1)
	(4) RS=1时返回栈开启
	(5) BPE=1时预测跳转开启
	(6) BTB=1时分支目标预测开启
	(8) WBR=1时支持写突发传输写 （c906固定为1）
	(12) L0BTB=1时第一级分支目标预测开启
	*/
	csr_set(CSR_MHCR, 0x11fc);


	/*
	(15) MM为1时支持非对齐访问，硬件处理非对齐访问
	(16) UCME为1时，用户模式可以执行扩展的cache操作指令
	(17) CLINTEE为1时，CLINT发起的超级用户软件中断和计时器中断可以被响应
	(21) MAEE为1时MMU的pte中扩展地址属性位，用户可以配置页面的地址属性
	(22) THEADISAEE为1时可以使用C906扩展指令集
	*/
	csr_set(CSR_MXSTATUS, 0x638000);


	/*
	(2) DPLD=1，dcache预取开启
	(3,4,5,6,7) AMR=1，时，在出现连续3条缓存行的存储操作时后续连续地址的存储操作不再写入L1Cache
	(8) IPLD=1ICACHE预取开启
	(9) LPE=1循环加速开启
	(13,14) DPLD为2时，预取8条缓存行
	*/
	csr_write(CSR_MHINT, 0x16e30c);
}

void dcache_enable(void)
{
    /*
    (1) DE=1时Dcache打开
    */
    csr_set(CSR_MHCR, 0x2);
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
