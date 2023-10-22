#include <FreeRTOS.h>
#include <task.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <console.h>

#include "../FreeRTOS_CLI.h"

#include <aw_malloc.h>

extern size_t xPortGetTotalHeapSize( void );

#ifdef CONFIG_MEMMANG_HEAP_MULTIPLE

static void show_heap_info(int heapID, uint32_t totalsize, char *ram_name)
{
    uint32_t freesize = 0;
    uint32_t minfreesize = 0;

    freesize = aw_xPortGetFreeHeapSize(heapID);
    minfreesize = aw_xPortGetMinimumEverFreeHeapSize(heapID);

    printf( "%s Heap:\n", ram_name);
    printf( "    Total Size : %8d Bytes    (%5d KB)\n"
            "          Free : %8d Bytes    (%5d KB)\n"
            "      Min Free : %8d Bytes    (%5d KB)\n",
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

    printf( "Total Heap Size : %8d Bytes    (%5d KB)\n"
        "           Free : %8d Bytes    (%5d KB)\n"
        "       Min Free : %8d Bytes    (%5d KB)\n",
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

#ifndef CONFIG_MEMMANG_HEAP_MULTIPLE
        show_heap_info();
#endif
        if (i + 1 != num)
            vTaskDelay(configTICK_RATE_HZ);
    }

    return 0;
}

FINSH_FUNCTION_EXPORT_CMD(cmd_free, free, Free Memory in Heap);
