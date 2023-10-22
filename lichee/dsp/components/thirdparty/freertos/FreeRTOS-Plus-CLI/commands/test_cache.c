#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hal_log.h>
#include <hal_interrupt.h>
#include <hal_cmd.h>
#include <hal_mem.h>
#include <hal_cache.h>
#include <hal_dma.h>
#include <sunxi_hal_common.h>

#define DCACHE_TEST_LEN 1024

extern void test_modify_icache(void);

static void dma_test_cb(void *param)
{
}

__attribute__((aligned(64))) void print_icache_test(void)
{
    printf("%s, %d\n", __func__, __LINE__);
}

static int cmd_test_icache(int argc, char **argv)
{
	char *func_addr = (char *)&print_icache_test;
	volatile uint16_t *addr = 0;
	int flag = 0;
	unsigned long cpsr;

	printf("Usage:test_icache [inv] \r\n\r\n");

	if (argc == 2 && !strcmp("inv", argv[1]))
		flag = 1;

	if ((unsigned long)func_addr % 2) {
        addr = (uint16_t *)(func_addr - 1);
	} else {
        addr = (uint16_t *)func_addr;
	}

    print_icache_test();

	cpsr = hal_interrupt_disable_irqsave();
#ifdef CONFIG_ARCH_RISCV
    /* set ebreak instruction */
    *(volatile uint16_t *) addr = (0x9002);
#elif defined(CONFIG_ARCH_DSP)
	volatile uint32_t *addr_32 = (volatile uint32_t *)addr;
    *(volatile uint32_t *) addr_32 = (0x00000000);
#elif defined(CONFIG_ARCH_ARM)
    /* set udf instruction */
    *(volatile uint16_t *) addr = (0xde00);
#endif

    hal_dcache_clean((unsigned long)addr, DCACHE_TEST_LEN);

	dsb();
	isb();

    if (flag == 1)
		hal_icache_invalidate_all();

    print_icache_test();

    hal_interrupt_enable_irqrestore(cpsr);

    printf("test icache successfully!\r\n");
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_test_icache, test_icache, icache tests)

#ifdef CONFIG_DRIVERS_DMA
static int cmd_test_dcache(int argc, char **argv)
{
    int ret, i;
    struct sunxi_dma_chan *hdma = NULL;
    char *src = NULL, *dst = NULL;
    char *src_rev = NULL;
    char *dst_rev = NULL;
    struct dma_slave_config config = {0};
    uint32_t size = 0;

    dst = hal_malloc_coherent(DCACHE_TEST_LEN);
    src = hal_malloc_coherent(DCACHE_TEST_LEN);

    dst_rev = hal_malloc_coherent(DCACHE_TEST_LEN);
    src_rev = hal_malloc_coherent(DCACHE_TEST_LEN);

    if (src == NULL || dst == NULL || dst_rev == NULL || src_rev == NULL)
    {
        printf("malloc src error!");
        goto end;
    }

    memset(src, 0, DCACHE_TEST_LEN);
    memset(dst, 0, DCACHE_TEST_LEN);
    memset(src_rev, 0, DCACHE_TEST_LEN);
    memset(dst_rev, 0, DCACHE_TEST_LEN);

    for (i = 0; i < DCACHE_TEST_LEN; i++)
    {
        src[i] = i & 0xff;
    }

    memcpy(src_rev, src, DCACHE_TEST_LEN);
    memcpy(dst_rev, dst, DCACHE_TEST_LEN);

    hal_dcache_clean((unsigned long)src, DCACHE_TEST_LEN);
    //hal_dcache_clean(dst, DCACHE_TEST_LEN);

    if (memcmp(dst_rev, dst, DCACHE_TEST_LEN))
    {
        printf("test1: meet error, clean dcache range failed !!!!\n");
        printf("dst buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", dst[i]);
        }

        printf("\ndst_rev buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", dst_rev[i]);
        }
        printf("\n\n");
    }
    else
    {
        printf("test1: dcache clean range work but not mean successfull!\n");
    }

    /* request dma chan */
    ret = hal_dma_chan_request(&hdma);
    if (ret == HAL_DMA_CHAN_STATUS_BUSY)
    {
        printf("dma channel busy!");
        goto end;
    }

    /* register dma callback */
    ret = hal_dma_callback_install(hdma, dma_test_cb, hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("register dma callback failed!");
        goto end;
    }

    config.direction = DMA_MEM_TO_MEM;
    config.dst_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
    config.src_addr_width = DMA_SLAVE_BUSWIDTH_8_BYTES;
    config.dst_maxburst = DMA_SLAVE_BURST_16;
    config.src_maxburst = DMA_SLAVE_BURST_16;
    config.slave_id = sunxi_slave_id(DRQDST_SDRAM, DRQSRC_SDRAM);

    ret = hal_dma_slave_config(hdma, &config);

    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma config error, ret:%d", ret);
        goto end;
    }

    ret = hal_dma_prep_memcpy(hdma, (uint32_t)dst, (uint32_t)src, DCACHE_TEST_LEN);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma prep error, ret:%d", ret);
        goto end;
    }

    ret = hal_dma_start(hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma start error, ret:%d", ret);
        goto end;
    }

    while (hal_dma_tx_status(hdma, &size) != 0);

    ret = hal_dma_stop(hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma stop error, ret:%d", ret);
        goto end;
    }

    ret = hal_dma_chan_free(hdma);
    if (ret != HAL_DMA_STATUS_OK)
    {
        printf("dma free error, ret:%d", ret);
        goto end;
    }

    if (memcmp(dst, dst_rev, DCACHE_TEST_LEN))
    {
        printf("test2: meet error, dcache maybe not open or dma work faild !!!!\n");
        printf("src buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", src[i]);
        }

        printf("\ndst buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", dst[i]);
        }
        printf("\n\n");
    }
    else
    {
        printf("test2: dcache open, dcache clean range successfully!i\n");
    }

    hal_dcache_invalidate((unsigned long)src, DCACHE_TEST_LEN);
    hal_dcache_invalidate((unsigned long)dst, DCACHE_TEST_LEN);

    if (memcmp(dst, src, DCACHE_TEST_LEN))
    {
        printf("test3: meet error, invalidate dcache failed !!!!\n");
        printf("src buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", src[i]);
        }

        printf("\ndst buf:\n");
        for (i = 0; i < DCACHE_TEST_LEN; i++)
        {
            printf("0x%x ", dst[i]);
        }
        printf("\n\n");
    }
    else
    {
        printf("test3: dcache invalidate range successfully!\n");
    }

end:
    if (src)
    {
        hal_free_coherent(src);
    }

    if (dst)
    {
        hal_free_coherent(dst);
    }

    if (dst_rev)
    {
        hal_free_coherent(dst_rev);
    }

    if (src_rev)
    {
        hal_free_coherent(src_rev);
    }

    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_test_dcache, test_dcache, dcache tests)
#endif
