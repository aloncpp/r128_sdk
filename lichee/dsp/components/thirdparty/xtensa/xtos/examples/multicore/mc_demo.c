
// Copyright (c) 2018 Cadence Design Systems, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// XTOS single-image multicore example.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <xtensa/config/core.h>
#include <xtensa/config/system.h>
#include <xtensa/tie/xt_core.h>
#include <xtensa/xtruntime.h>
#include <xtensa/hal.h>


#if XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_RAM_SIZE > 0)

// Set USE_MUTEX to zero to disable use of XTOS mutex. The difference
// will be seen in the stdout output.
#ifndef USE_MUTEX
#define USE_MUTEX    1
#endif

// Print macro for convenience.
#define PRINT(...)    { printf("core%d: ", prid); printf(__VA_ARGS__); }

// Hardwired address for slave reset register (see mmio.txt).
#define SLAVE_RESET_REG_ADDR    ((uint32_t *) 0xF0000000)

// Default number of cores in system.
#define NUM_CORES               4

// If there is iram, place the interrupt handler in iram. For romable
// executables this will verify that iram was unpacked properly by all
// the cores.
#if (XCHAL_NUM_INSTRAM > 0)
static void
sw_handler(void * arg) __attribute__ ((section(".iram0.text")));
#endif

// Software interrupt handler.
static void
sw_handler(void * arg)
{
    uint32_t prid = XT_RSR_PRID();
    uint32_t arid = *(uint32_t *) arg;

    if (XT_RSR_PRID() != arid) {
        PRINT("invalid arg for sw handler\n");
        exit(-1);
    }

    PRINT("sw int handler called arg %p *arg %u\n", arg, arid);
}


// Find a software interrupt and exercise it, to show that each
// core sets and calls its own handler.
static int32_t
check_sw_interrupt(void)
{
    int32_t  i;
    int32_t  intnum = -1;
    uint32_t prid   = XT_RSR_PRID();

    for (i = 0; i < XCHAL_NUM_INTERRUPTS; i++) {
        if (Xthal_inttype[i] == XTHAL_INTTYPE_SOFTWARE) {
            intnum = i;
            break;
        }
    }

    if (intnum == -1) {
        PRINT("no sw interrupts\n");
        return 0;
    }

    xtos_set_interrupt_handler(intnum, sw_handler, (void *) &prid, NULL);
    xtos_interrupt_enable(intnum);
    xthal_interrupt_trigger(intnum);

    xtos_interrupt_disable(intnum);
    return 0;
}


// Global objects shared by all cores.
xtos_barrier bar0;
xtos_barrier bar1;
#if USE_MUTEX
xtos_mutex   mtx0;
#endif


int
main(int argc, char *argv[])
{
    int32_t  i;
    int32_t  ret;
    uint32_t prid = XT_RSR_PRID();

    if (prid == 0) {
        // Core 0 is the master...
#if USE_MUTEX
        // Init the mutex
        xtos_mutex_init(&mtx0);
#endif
        // Init the barriers we will use
        xtos_barrier_init(&bar0, NUM_CORES);
        xtos_barrier_init(&bar1, NUM_CORES);

        // Release other cores from reset
        *(SLAVE_RESET_REG_ADDR) = 0;
    }

    // Synchronize at the barrier.
    ret = xtos_barrier_sync(&bar0);
    if (ret != 0) {
        fprintf(stderr, "Barrier sync error\n");
        return -1;
    }

    // When we get here, this core is in a mutual exclusion region,
    // since all other cores are either at barrier_sync or barrier_wait.
    PRINT("at barrier\n");
    {
        extern void * _heap_sentry_ptr;
        PRINT("heap end at %p\n", _heap_sentry_ptr); fflush(stdout);
    }

    // Wait for everyone to arrive at the barrier.
    ret = xtos_barrier_wait(&bar0);
    if (ret != 0) {
        fprintf(stderr, "Barrier wait error\n");
        return -1;
    }

#if USE_MUTEX
    xtos_mutex_lock(&mtx0);
#endif

    PRINT("hello I am core %u\n", prid);

    printf("args: ");
    for (i = 0; i < argc; i++) {
        printf("<%s> ", argv[i]);
    }
    printf("\n");

    PRINT("stack is %p\n", &prid);

    check_sw_interrupt();

#if USE_MUTEX
    xtos_mutex_unlock(&mtx0);
#endif

    // Sync again at the exit barrier. This prevents one core from
    // terminating the simulation and closing global file handles
    // before the others are done using them.
    xtos_barrier_sync(&bar1);
    PRINT("at exit barrier\n");
    xtos_barrier_wait(&bar1);

    return 0;
}

// Override the default MPU setup. This table matches the memory map
// of the 'sample_controller' core and will need to be modified for
// other cores.
// NOTE: This table sets up all of external memory as shared uncached.
// For best results, edit the LSP memory map to create a separate 
// section in shared memory, place all sections that need to be uncached
// into that section, and only map that section uncached. See README
// for more details.

const unsigned int
__xt_mpu_init_table_size __attribute__((section(".ResetHandler.text"))) = 5;

const struct xthal_MPU_entry
__xt_mpu_init_table[5] __attribute__((section(".ResetHandler.text"))) =
{
  XTHAL_MPU_ENTRY(0x00000000, 1, XTHAL_AR_NONE,   XTHAL_MEM_DEVICE),
  XTHAL_MPU_ENTRY(0x20000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_DEVICE),
  XTHAL_MPU_ENTRY(0x50000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_WRITEBACK),
  XTHAL_MPU_ENTRY(0x60000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_NON_CACHEABLE | XTHAL_MEM_SYSTEM_SHAREABLE),
  XTHAL_MPU_ENTRY(0x80000000, 1, XTHAL_AR_RWXrwx, XTHAL_MEM_DEVICE)
};

#else

#error "Processor ID, Exclusive Store, and External Memory are all required for multicore".

#endif // XCHAL_HAVE_PRID && XCHAL_HAVE_EXCLUSIVE && (XSHAL_RAM_SIZE > 0)

