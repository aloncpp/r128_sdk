#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>
#include <mmu_cache.h>
#include <io.h>

#include <hal_cache.h>
#include <hal_cmd.h>
#include <hal_interrupt.h>

#include <errno.h>

#ifdef CONFIG_DRIVERS_RTC
#include <sunxi_hal_rtc.h>
#endif

#ifdef CONFIG_DRIVERS_WATCHDOG
#include <sunxi_hal_watchdog.h>
#endif

#ifdef CONFIG_DRIVERS_PRCM
#include <sunxi_hal_prcm.h>
#endif

#ifdef CONFIG_DRIVERS_SPINOR
#include <sunxi_hal_spinor.h>
#endif

#include <time.h>
#include <sys/time.h>

#ifdef CONFIG_COMMAND_KTIME
#include <ktimer.h>
#endif

#ifdef CONFIG_COMPONENTS_BOOT_REASON
#include <boot_reason.h>
#endif

#include <aw_version.h>
#include <aw_malloc.h>

#include <task.h>

int nor_write(unsigned int addr, char *buf, unsigned int len);
int nor_read(unsigned int addr, char *buf, unsigned int len);
int nor_erase(unsigned int addr, unsigned int size);

#define BUF_SIZE 8192

#ifdef CONFIG_COMMAND_TASK_STATUS
int cmd_ts(int argc, char **argv)
{
    char *const pcHeader = "Task          State  Priority  Stack  #\r\n************************************************\r\n";
    char pcWriteBuffer[BUF_SIZE] = {0};

    /* Generate a table of task stats. */
    strcpy( pcWriteBuffer, pcHeader );
    vTaskList( pcWriteBuffer + strlen( pcHeader ) );
    printf("%s", pcWriteBuffer);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_ts, __cmd_ts, Display a table showing the state of each FreeRTOS task);

int cmd_run_time_stats(int argc, char **argv)
{
    char *const pcHeader = "Task            Abs Time      % Time\r\n****************************************\r\n";
    char pcWriteBuffer[BUF_SIZE] = {0};

    /* Generate a table of task stats. */
    strcpy( pcWriteBuffer, pcHeader );
    vTaskGetRunTimeStats( ( char * ) pcWriteBuffer + strlen( pcHeader ) );
    printf("%s", pcWriteBuffer);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_run_time_stats, __cmd_run_time_stats, Display processing time each FreeRTOS task has used);
#endif

#ifdef CONFIG_COMMAND_SAMPLE
int cmd_sample(int argc, char ** argv)
{
    int i = 0;
    int ch = 0;
    int temp_argc = argc;

    /* sample 1 2 3 */
    while(temp_argc--)
    {
        printf("argv[%d] = %s\n", i, argv[i]);
        i++;
    }

    /* sample -a 1 -b 2 -c 3*/
    printf("\noption argument:\n");
    temp_argc = argc;
    while((ch = getopt(temp_argc, argv, "a:b:c:")) != -1)
    {
        switch(ch)
        {
            case 'a':
                printf("-a = %s\n", optarg);
                break;
            case 'b':
                printf("-b = %s\n", optarg);
                break;
            case 'c':
                printf("-c = %s\n", optarg);
                break;
            default:
                break;
        }
    }

    /* sample --add=1 --bt=2 --create=3*/
    printf("\nlong option argument:\n");
    ch = 0;
    temp_argc = argc;
    int option_index = 0;
    struct option long_options[] =
    {
        {"add", 1, NULL, 'a'},
        {"bt", 1, NULL, 'b'},
        {"create", 1, NULL, 'c'},
        {0, 0, 0, 0},
    };

    while((ch = getopt_long(argc, argv, "a:b:c:", long_options, &option_index)) != -1)
    {
        switch(ch)
        {
            case 'a':
                printf("-a = %s\n", optarg);
                break;
            case 'b':
                printf("-b = %s\n", optarg);
                break;
            case 'c':
                printf("-c = %s\n", optarg);
                break;
            default:
                break;
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_sample, sample, Console Sample Command);
#endif

#ifdef CONFIG_COMMAND_VERSION
char *get_kernel_version(void)
{
    return tskKERNEL_VERSION_NUMBER;
}

char *get_tina_rtos_version(void)
{
    return TINA_RT_VERSION_NUMBER;
}

int cmd_version(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    printf("AW Tina-RTOS Version : %s\n", get_tina_rtos_version());
    printf("AW Tina-RTOS Kernel(FreeRTOS) Version : %s\n", get_kernel_version());
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_version, version, Get Version Information);
#endif

#ifdef CONFIG_COMMAND_REBOOT
int cmd_reboot(int argc, char ** argv)
{
    int i = 0;
    if(argc == 2) {
        if(!strcmp(argv[1],"efex")) {
            printf("**** Jump to efex! *****\n");
            hal_rtc_set_fel_flag();
        }
    }
#ifdef CONFIG_DRIVERS_SPINOR_CACHE
    void NorSyncTimerStop(void);
    NorSyncTimerStop();
#endif
#ifdef CONFIG_DRIVERS_SPINOR
    hal_spinor_deinit();
#endif
#ifdef CONFIG_COMPONENTS_BOOT_REASON
	app_write_boot_reason_when_reboot();
#endif

#ifdef CONFIG_ARCH_SUN20IW2
#define CCMU_AON_BASE 0x4004c400
    hal_interrupt_disable();
    /* force ble reset */
    sr32(CCMU_AON_BASE + 0xc8, 16, 1, 0x0);
    /* force rfas reset */
    sr32(CCMU_AON_BASE + 0xc8, 9, 1, 0x0);
    /* force wlan soc reset */
    sr32(CCMU_AON_BASE + 0xc8, 12, 1, 0x0);
    /* disable wlan cpu clk */
    sr32(CCMU_AON_BASE + 0xcc, 12, 1, 0x0);
    HAL_PRCM_EnableWlanCPUClk(1); /* '1' is disable */
#undef CCMU_AON_BASE
#endif
    hal_watchdog_restart();
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_reboot, reboot, Console reboot Command);
#endif

#ifdef CONFIG_COMMAND_UPGRADE
int cmd_upgrade(int argc, char ** argv)
{
	HAL_PRCM_SetCPUBootFlag(PRCM_CPU_TYPE_INDEX_AR800A, PRCM_CPU_BOOT_FROM_SYS_UPDATE);
	hal_watchdog_restart();
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_upgrade, upgrade, Console upgrade Command);
#endif

#ifdef CONFIG_COMMAND_HELP
int cmd_help(int argc, char ** argv)
{
    printf("Lists all the registered commands\n");
    printf("\n");

#ifdef CONFIG_COMPONENT_CLI
    typedef struct xCOMMAND_INPUT_LIST
    {
        const CLI_Command_Definition_t *pxCommandLineDefinition;
        struct xCOMMAND_INPUT_LIST *pxNext;
    } CLI_Definition_List_Item_t;

    extern CLI_Definition_List_Item_t xRegisteredCommands;
    CLI_Definition_List_Item_t * pxCommand = &xRegisteredCommands;
    while(pxCommand != NULL)
    {
        printf("[%20s]--------------%s\n",
            pxCommand->pxCommandLineDefinition->pcCommand,
            pxCommand->pxCommandLineDefinition->pcHelpString);
        printf("\n");
        pxCommand = pxCommand->pxNext;
    }
#endif

    finsh_syscall_show();

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_help, help, List all registered commands);
#endif

#ifdef CONFIG_COMMAND_MMC_READ
int cmd_mmc_read(int argc, char **argv)
{
    uint32_t addr;
    char *err = NULL;
    uint32_t len;
    char *buf;

    if (argc < 3) {
        printf("Usage:nor_read addr len (sector)!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);

    buf = malloc(len * 512);
    if (!buf) {
        printf("alloc memory failed!\n");
        return -1;
    }
    memset(buf, 0, len * 512);
    struct mmc_card *card = mmc_card_open(0);
    mmc_block_read(card, buf, addr, len);
    mmc_card_close(0);

    printf("====================\n");
    aw_hexdump(buf, len * 512);
    printf("====================\n");

    free(buf);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_mmc_read, mmc_read, nor read);
#endif

#ifdef CONFIG_COMMAND_MMC_WRITE
int cmd_mmc_write(int argc, char **argv)
{
    uint32_t addr;
    char *err = NULL;
    uint32_t len;
    char *buf;
    uint32_t value;

    if (argc < 4) {
        printf("Usage:mmc_write addr len value (sector)!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);
    value = strtoul(argv[3], &err, 0);

    buf = malloc(len * 512);
    if (!buf) {
        printf("alloc memory failed!\n");
        return -1;
    }
    memset(buf, value, len * 512);
    struct mmc_card *card = mmc_card_open(0);
    mmc_block_write(card, buf, addr, len);
    mmc_card_close(0);

    free(buf);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_mmc_write, mmc_write, Performance monitor);
#endif

#ifdef CONFIG_COMMAND_NOR_READ
int cmd_nor_read(int argc, char **argv)
{
    uint32_t addr;
    char *err = NULL;
    uint32_t len;
    char *buf;

    if (argc < 3) {
        printf("Usage:nor_read addr len!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);

    buf = malloc(len);
    if (!buf) {
        printf("alloc memory failed!\n");
        return -1;
    }
    memset(buf, 0, len);
    nor_read(addr, buf, len);

    printf("====================\n");
    aw_hexdump(buf, len);
    printf("====================\n");
    free(buf);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_nor_read, nor_read, nor read);
#endif

#ifdef CONFIG_COMMAND_NOR_ERASE
int cmd_nor_erase(int argc, char **argv)
{
    uint32_t addr;
    char *err = NULL;
    uint32_t len;

    if (argc < 3) {
        printf("Usage:nor_erase addr len!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);

    nor_erase(addr, len);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_nor_erase, nor_erase, nor erase);
#endif

#ifdef CONFIG_COMMAND_NOR_WRITE
int cmd_nor_write(int argc, char **argv)
{
    uint32_t addr;
    char *err = NULL;
    uint32_t len;
    char *buf;
    uint32_t value;

    if (argc < 4) {
        printf("Usage:nor_write addr len value!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);
    value = strtoul(argv[3], &err, 0);

    buf = malloc(len);
    if (!buf) {
        printf("alloc memory failed!\n");
        return -1;
    }
    memset(buf, value, len);
    nor_write(addr, buf, len);
    free(buf);

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_nor_write, nor_write, Performance monitor);
#endif

__attribute__((weak)) int xport_is_valid_address(void* start_addr, void* end_addr)
{
    return 1;
}

#ifdef CONFIG_COMMAND_DCACHE_CLEAN
int cmd_dcache_clean(int argc, char ** argv)
{
    unsigned long addr, len;
    char *err = NULL;

    if(argc < 3)
    {
        printf("Argument Error!\n");
        return -1;
    }

    if ((NULL == argv[1]) || (NULL == argv[2]))
    {
        printf("Argument Error!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);

    if(xport_is_valid_address((void *)addr, NULL))
    {
        hal_dcache_clean(addr, len);
    }
    else
    {
        printf("Invalid address!\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_dcache_clean, dcache_clean, clean dcache to memory);
#endif

#ifdef CONFIG_COMMAND_DCACHE_INVALIDATE
int cmd_dcache_invalidate(int argc, char ** argv)
{
    unsigned long addr, len;
    char *err = NULL;

    if(argc < 3)
    {
        printf("Argument Error!\n");
        return -1;
    }

    if ((NULL == argv[1]) || (NULL == argv[2]))
    {
        printf("Argument Error!\n");
        return -1;
    }

    addr = strtoul(argv[1], &err, 0);
    len = strtoul(argv[2], &err, 0);

    if(xport_is_valid_address((void *)addr, NULL))
    {
        hal_dcache_invalidate(addr, len);
    }
    else
    {
        printf("Invalid address!\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_dcache_invalidate, dcache_invalidate, invalidate dcache);
#endif

#ifdef CONFIG_COMMAND_REG_WRITE
int cmd_reg_write(int argc, char ** argv)
{
    unsigned long reg_addr, reg_value;
    char *err = NULL;

    if(argc < 3)
    {
        printf("Argument Error!\n");
        return -1;
    }

    if ((NULL == argv[1]) || (NULL == argv[2]))
    {
        printf("Argument Error!\n");
        return -1;
    }

    reg_addr = strtoul(argv[1], &err, 0);
    reg_value = strtoul(argv[2], &err, 0);

    if(xport_is_valid_address((void *)reg_addr, NULL))
    {
        *((volatile uint32_t *)(reg_addr)) = reg_value;
    }
    else
    {
        printf("Invalid address!\n");
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_reg_write, m, write value to register: m reg_address reg_value);
#endif

#ifdef CONFIG_COMMAND_REG_READ
int cmd_reg_read(int argc, char ** argv)
{
    unsigned long reg_addr;
    char *err = NULL;
    unsigned long start_addr, end_addr;
    uint32_t len;

    if (NULL == argv[1])
    {
        printf("Argument Error!\n");
        return -1;
    }

    if (argv[2])
    {
        start_addr = strtoul(argv[1], &err, 0);

        len = strtoul(argv[2], &err, 0);
        end_addr = start_addr + len;

        if(xport_is_valid_address((void *)start_addr, (void *)end_addr) && end_addr != 0)
        {
            printf("start_addr=0x%08lx end_addr=0x%08lx\n", start_addr, end_addr);
            for (; start_addr <= end_addr;)
            {
                printf("reg_addr[0x%08lx]=0x%08x \n", start_addr, *((volatile uint32_t *)(start_addr)));
                start_addr += 4;
            }
        }
        else
        {
            printf("Invalid address!\n");
        }
    }
    else
    {
        reg_addr = strtoul(argv[1], &err, 0);
        if(xport_is_valid_address((void *)reg_addr, NULL))
        {
            printf("reg_addr[0x%08lx]=0x%08x \n", reg_addr, *((volatile uint32_t *)(reg_addr)));
        }
        else
        {
            printf("Invalid address!\n");
        }
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_reg_read, p, read value from register: p reg_start_addr len);
#endif

#ifdef CONFIG_COMMAND_CLEAN
int cmd_clean(int argc, char ** argv)
{
    printf("\e[1;1H\e[2J");
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_clean, clean, Clean terminal);
#endif

#ifdef CONFIG_COMMAND_BOOT_C906
int cmd_load_c906(int argc, char ** argv)
{
    int ret;
    int fd = 0;
    int pos = 0;
    char *file = "/dev/rv-sram";
    char buf[256];
    char *loadaddr = (void *)(CONFIG_ARCH_RISCV_START_ADDRESS);

    struct stat s;

    printf("%s loadaddr to 0x%p\n", file, loadaddr);
    ret = stat(file, &s);
    if (ret) {
        printf("stat %s failed - %s\n", file, strerror(errno));
        return -1;
    }

    fd = open(file, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed - %s\n", file, strerror(errno));
        return -1;
    }

    while ((ret = read(fd, buf, 256))) {
        if (ret < 0) {
            printf("read %s failed - %s\n", file, strerror(errno));
            goto close;
        }
        memcpy(loadaddr + pos, buf, ret);
        pos += ret;
    }
    printf("clean RV START_ADDRESS dcache...\n");
    hal_dcache_clean(CONFIG_ARCH_RISCV_START_ADDRESS, 1024 * 1024);
close:
    close(fd);

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_load_c906, load_c906, load c906);

int cmd_boot_c906(int argc, char ** argv)
{
    sr32(0x40051400+0x100,8,1,1); //rv wakeup enable
    while((readl(0x40051400+0x104)&(1<<8)) == 0); //wait rv alive
    sr32(0x4003C000+0x064,31,1,1); //rv clk enable
    sr32(0x4003C000+0x014,19,1,1); //rv clk gating
    sr32(0x4003C000+0x018,19,1,1); //rv clk rst
    sr32(0x4003C000+0x018,21,1,1); //rv sys apb soft rst
    put_wvalue(0x40028000+0x004,CONFIG_ARCH_RISCV_START_ADDRESS); //rv start address
    sr32(0x4003C000+0x18,16,1,1); //rv core reset
}
FINSH_FUNCTION_EXPORT_CMD(cmd_boot_c906, boot_c906, boot c906);
#endif

#ifdef CONFIG_COMMAND_BOOT_DSP
extern int sun20i_boot_dsp(void);
extern int sun20i_boot_dsp_with_start_addr(uint32_t dsp_start_addr);
int cmd_boot_dsp(int argc, char ** argv)
{
	char *ptr = NULL;
	errno = 0;

	if (argc == 1)
	{
		sun20i_boot_dsp();
		return 0;
	}

	if (argc != 2)
	{
		printf("invalid input parameter num(%d)!\n", argc);
		return 0;
	}

	uint32_t addr = strtoul(argv[1], &ptr, 0);
	if (errno || (ptr && *ptr != '\0'))
	{
		printf("invalid input parameter('%s')!\n", argv[1]);
		return 0;
	}

	sun20i_boot_dsp_with_start_addr(addr);
	return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_boot_dsp, boot_dsp, boot dsp);
#endif

#ifdef CONFIG_COMMAND_PANIC
int cmd_panic(int argc, char ** argv)
{
#ifdef CONFIG_ARCH_ARM_ARMV8M
    __asm__ volatile("udf \r\n" :::"memory");
#elif defined CONFIG_ARCH_RISCV
    __asm__ volatile("ebreak \r\n" :::"memory");
#else
    printf("data = %d\n", *(unsigned int *)0x90000000);
#endif
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_panic, panic, enter panic mode);
#endif

#ifdef CONFIG_COMMAND_MEM_LAYOUT
int cmd_memlay(int argc, char ** argv)
{
    extern uint32_t __head_start, __head_end;
    extern uint32_t __text_start, __text_end;
//    extern uint32_t __init_start, __init_end;
//    extern uint32_t __fini_start, __fini_end;
    extern uint32_t __rodata_start, __rodata_end;
//    extern uint32_t __rodata1_start, __rodata1_end;
//    extern uint32_t __sdata2_start, __sdata2_end;
//    extern uint32_t __sbss2_start, __sbss2_end;
    extern uint32_t __data_start, __data_end;
//    extern uint32_t __data1_start, __data1_end;
//    extern uint32_t __got_start, __got_end;
//    extern uint32_t __CTOR_LIST__, __CTOR_END__;
//    extern uint32_t __DTOR_LIST__, __DTOR_END__;
//    extern uint32_t __fixup_start, __fixup_end;
//    extern uint32_t __en_frame_start, __en_frame_end;
//    extern uint32_t __eh_framehdr_start, __eh_framehdr_end;
//    extern uint32_t __gcc_except_table_start, __gcc_except_table_end;
//    extern uint32_t __mmu_tbl_start, __mmu_tbl_end;
    extern uint32_t __exidx_start, __exidx_end;
//    extern uint32_t __preinit_array_start, __preinit_array_end;
//    extern uint32_t __init_array_start, __init_array_end;
//    extern uint32_t __fini_array_start, __fini_array_end;
//    extern uint32_t __attributes_start, __attributes_end;
//    extern uint32_t __sdata_start, __sdata_end;
//    extern uint32_t _syscall_table_begin, _syscall_table_end;
    extern uint32_t _tt_begin, _tt_end;
    extern uint32_t _stack_end, _stack_start;
    extern uint32_t __tlb_base_entry, __tlb_base_end;
//    extern uint32_t __sbss_start, __sbss_end;
//    extern uint32_t __tdata_start, __tdata_end;
//    extern uint32_t __tbss_start, __tbss_end;
    extern uint32_t __bss_start, __bss_end;
    extern uint32_t _heap_start, _heap_end;

#define MLB(b, e) b, e, ((unsigned long)(e) - (unsigned long)(b)) << 2
#define MLK(b, e) b, e, (((unsigned long)(e) - (unsigned long)(b)) << 2) >> 10
//#define MLM(b, e) b, e, ((e) - (b)) >> 20

    uint32_t heap_start = (uint32_t)&_heap_start;
    uint32_t heap_end = (uint32_t)&_heap_end;

    if(heap_end > CONFIG_VIRTUAL_DRAM_ADDR + CONFIG_DRAM_SIZE)
    {
        heap_end = CONFIG_VIRTUAL_DRAM_ADDR + CONFIG_DRAM_SIZE;
    }

    printf("Kernel Memory Layout:\n"
        "          .head : %08p - %08p   (%8ld Bytes)\n"
        "          .text : %08p - %08p   (%8ld Bytes)\n"
        "        .rodata : %08p - %08p   (%8ld Bytes)\n"
        "          .data : %08p - %08p   (%8ld Bytes)\n"
        "     .ARM.exidx : %08p - %08p   (%8ld Bytes)\n"
        "       .FSymTab : %08p - %08p   (%8ld Bytes)\n"
        "        .ttcall : %08p - %08p   (%8ld Bytes)\n"
        "         .stack : %08p - %08p   (%8ld Bytes)\n"
        "      .tlb_base : %08p - %08p   (%8ld Bytes)\n"
        "           .bss : %08p - %08p   (%8ld Bytes)\n"
        "          .heap : %08p - %08p   (%8ld Bytes)\n",
        MLB(&__head_start, &__head_end),
        MLB(&__text_start, &__text_end),
        MLB(&__rodata_start, &__rodata_end),
        MLB(&__data_start, &__data_end),
        MLB(&__exidx_start, &__exidx_end),
        MLB(&_syscall_table_begin, &_syscall_table_end),
        MLB(&_tt_begin, &_tt_end),
        MLB(&_stack_end, &_stack_start),
        MLB(&__tlb_base_entry, &__tlb_base_end),
        MLB(&__bss_start, &__bss_end),
        MLB((uint32_t *)heap_start, (uint32_t *)heap_end));

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_memlay, memlay, Kernel Memory Layout);
#endif

#ifdef CONFIG_COMMAND_FREE

#ifdef CONFIG_HEAP_MULTIPLE
static void show_heap_info(int heapID, uint32_t totalsize, char *ram_name)
{
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;

    totalsize = aw_xPortGetTotalHeapSize(heapID);
    freesize = aw_xPortGetFreeHeapSize(heapID);
    minfreesize = aw_xPortGetMinimumEverFreeHeapSize(heapID);

    printf( "%s Heap:\n", ram_name);
    printf( "    Total Size : %8ld Bytes    (%5ld KB)\n"
            "          Free : %8ld Bytes    (%5ld KB)\n"
            "      Min Free : %8ld Bytes    (%5ld KB)\n",
        totalsize, totalsize >> 10,
        freesize, freesize >> 10,
        minfreesize, minfreesize >> 10);
    printf("\r\n");
}
#else
static void show_heap_info(void)
{
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;
	uint32_t totalsize = 0;

    freesize = xPortGetFreeHeapSize();
    minfreesize = xPortGetMinimumEverFreeHeapSize();
	totalsize = xPortGetTotalHeapSize();

    printf( "Total Heap Size : %8ld Bytes    (%5ld KB)\n"
        "           Free : %8ld Bytes    (%5ld KB)\n"
        "       Min Free : %8ld Bytes    (%5ld KB)\n",
        totalsize, totalsize >> 10,
        freesize, freesize >> 10,
        minfreesize, minfreesize >> 10);
    printf("\r\n");
}
#endif

int cmd_free(int argc, char ** argv)
{
    uint32_t i = 0;
    uint32_t num = 0;

    if (argc > 2)
        printf("Arguments Error!\nUsage: free <rounds>\n");
    else if (argc == 2)
        num = atoi(argv[1]);
    else
        num = 1;

    for (i = 0; i < num; i ++) {
        printf("==> Round [%d] <==\n", i+1);
#ifdef CONFIG_SRAM_HEAP
        show_heap_info(SRAM_HEAP_ID, CONFIG_SRAM_HEAP_SIZE, "sram");
#endif
#ifdef CONFIG_DRAM_HEAP
        show_heap_info(DRAM_HEAP_ID, CONFIG_DRAM_HEAP_SIZE, "dram");
#endif
#ifdef CONFIG_LPSRAM_HEAP
        show_heap_info(LPSRAM_HEAP_ID, CONFIG_LPSRAM_HEAP_SIZE, "lpsram");
#endif
#ifdef CONFIG_HPSRAM_HEAP
        show_heap_info(HPSRAM_HEAP_ID, CONFIG_HPSRAM_HEAP_SIZE, "hpsram");
#endif

#ifndef CONFIG_HEAP_MULTIPLE
        show_heap_info();
#endif
        char pcHeader[2048] = "Task          State  Priority  Stack      #\r\n************************************************\r\n";
        printf("\n      List Task MIN Free Stack(unit: word)      \n");
        vTaskList(pcHeader + strlen(pcHeader));
        printf("%s\n\n", pcHeader);

        if (i + 1 != num)
            vTaskDelay(configTICK_RATE_HZ);
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_free, free, Free Memory in Heap);
#endif

#ifdef CONFIG_COMMAND_KTIME
int cmd_ktime(int argc, const char **argv)
{
    struct timespec64 ts = {0LL, 0L};

    do_gettimeofday(&ts);

    printf("SEC: %lld, NSEC: %ld\n", ts.tv_sec, ts.tv_nsec);
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_ktime, time, ktime);
#endif

#ifdef CONFIG_COMMAND_TIMEDATECTL
static int cmd_timedatectl(int argc, char ** argv)
{
    struct timeval tm_val;
    struct tm tm;
    char *tz = NULL;

    char timestamp[50] = {0};

    tz = getenv("TZ");
    if (!tz)
        tz = "GMT";

    gettimeofday(&tm_val, NULL);

    localtime_r(&tm_val.tv_sec, &tm);
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        tm_val.tv_usec/1000);
    printf("%16s: %s %s\n", "Local time", timestamp, tz);

    memset(timestamp, 0, sizeof(timestamp));

    gmtime_r(&tm_val.tv_sec, &tm);
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
        tm.tm_year + 1900,
        tm.tm_mon + 1,
        tm.tm_mday,
        tm.tm_hour,
        tm.tm_min,
        tm.tm_sec,
        tm_val.tv_usec/1000);

    printf("%16s: %s %s\n", "Universal time", timestamp, "UTC");

    printf("%16s: %s(%d)\n", "Time Zone", tz, _timezone / 60 / 60);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_timedatectl, timedatectl, TimeDataCtl command);
#endif

#ifdef CONFIG_COMMAND_TZSET
static char *tz_name[]=
{
    "GMT",
    "UTC",
    "ECT",
    "EET",
    "ART",
    "EAT",
    "MET",
    "NET",
    "PLT",
    "IST",
    "BST",
    "VST",
    "CTT",
    "JST",
    "ACT",
    "AET",
    "SST",
    "NST",
    "MIT",
    "HST",
    "AST",
    "PST",
    "PNT",
    "MST",
    "CST",
    "EST",
    "IET",
    "PRT",
    "CNT",
    "AGT",
    "BET",
    "CAT",
};

static int cmd_tzset(int argc, char ** argv)
{
    int i;

    if (argc < 2) {
        printf("Usage: tzset tz_name\n");
        return -1;
    }

    for(i = 0; i < sizeof(tz_name)/sizeof(tz_name[0]); i++)
    {
        if (!strncmp(argv[1], tz_name[i], 3))
        {
            setenv("TZ", argv[1], 1);
            tzset();
            return 0;
        }
    }

    printf("Invalid Time Zone Name! Supported list are:\n");
    for(i = 0; i < sizeof(tz_name)/sizeof(tz_name[0]); i++)
    {
        printf("\t%s\n", tz_name[i]);
    }
    return -1;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_tzset, tzset, TimeZone Set command);
#endif

#ifdef CONFIG_COMMAND_DATE
static int getdayofmonth(int year, int month)
{
    if(month < 1 || month > 12)
    {
        return 0;
    }

    int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    days[1] = year % 4 == 0 && year % 100 || year % 400 == 0 ? 29 : 28;

    return days[month];
}

int cmd_date(int argc, char ** argv)
{
    struct timeval tm_val;
    struct tm tm;
    int ch = 0;
    int ret = 0;
    time_t time;
    char *tz = NULL;

    char timestamp[50] = {0};

    if(argc == 1)
    {
        gettimeofday(&tm_val, NULL);
        localtime_r(&tm_val.tv_sec, &tm);
        sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            tm_val.tv_usec/1000);

        tz = getenv("TZ");
        if (!tz)
            tz = "GMT";

        printf("%s %s\n", timestamp, tz);
        return ret;
    }

    while((ch = getopt(argc, argv, "s:")) != -1)
    {
        switch(ch)
        {
            case 's':
                memset(&tm, 0, sizeof(struct tm));
                ret = sscanf(optarg, "%04d-%02d-%02d %02d:%02d:%02d",
                        &tm.tm_year,
                        &tm.tm_mon,
                        &tm.tm_mday,
                        &tm.tm_hour,
                        &tm.tm_min,
                        &tm.tm_sec);

                if(ret != 6)
                {
                    printf("Argument Error!\n");
                    return -1;
                }
                else
                {
                    if((tm.tm_year < 1970)
                        || (tm.tm_mon < 1 || tm.tm_mon > 12)
                        || (tm.tm_mday < 1 || tm.tm_mday > getdayofmonth(tm.tm_year, tm.tm_mon))
                        || (tm.tm_hour < 0 || tm.tm_hour > 23)
                        || (tm.tm_min < 0 || tm.tm_min > 59)
                        || (tm.tm_sec < 0 || tm.tm_sec > 59))
                    {
                        printf("Argument Error!\n");
                        return -1;
                    }
                    tm.tm_year -= 1900;
                    tm.tm_mon -= 1;

                    time = mktime(&tm);

                    struct timeval sys_time = {0};
                    sys_time.tv_sec = time;
                    settimeofday(&sys_time, NULL);
                }
                break;
            default:
                break;
        }
    }
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_date, date, Date command);
#endif

#ifdef CONFIG_COMMAND_CLOCK_TIME
int cmd_ct(int argc, char ** argv)
{
    struct timespec xCurrentTime = {0};

    char timestamp[50];
    memset(timestamp, 0, sizeof(timestamp));

    if(argc == 1)
    {
        clock_gettime( CLOCK_REALTIME, &xCurrentTime  );
    }
    else if(!strcmp(argv[1], "r"))
    {
        clock_gettime( CLOCK_REALTIME, &xCurrentTime  );
    }
    else if(!strcmp(argv[1], "m"))
    {
        clock_gettime( CLOCK_MONOTONIC, &xCurrentTime  );
    }

    struct timeval tm_val;
    struct tm tm;

    tm_val.tv_sec = xCurrentTime.tv_sec;
    tm_val.tv_usec = xCurrentTime.tv_nsec / 1000;

    localtime_r(&tm_val.tv_sec, &tm);
    sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            tm.tm_year + 1900,
            tm.tm_mon + 1,
            tm.tm_mday,
            tm.tm_hour,
            tm.tm_min,
            tm.tm_sec,
            tm_val.tv_usec/1000);

    printf("clock get time = %s\n", timestamp);
}
FINSH_FUNCTION_EXPORT_CMD(cmd_ct, ct, Clock get time);
#endif
#ifdef CONFIG_COMMAND_LIST_IRQ
int cmd_list_irq(int argc, const char **argv)
{
extern void show_irqs(void);
	show_irqs();

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(cmd_list_irq, list_irq, list all enable interrupt);
#endif

