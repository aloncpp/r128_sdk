#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>

#include <console.h>
#include <csr.h>
#include "rv_pmu.h"

#include <aw_common.h>

static int exit_flag = 0;
static int gDelay = 0;

static int nEvent = 0;

struct event_counter_table
{
    int index;
    int event;
    int count;
    char *name;
};

#ifndef CONFIG_F133_SIM
static struct event_counter_table table_1[] =
{
    {
        .index = L1_ICACHE_ACCESS_COUNTER,
        .count = CSR_MHPMCOUNTER3,
        .event = CSR_MHPMEVENT3,
        .name = "l1_icache_access",
    },
    {
        .index = L1_ICACHE_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER4,
        .event = CSR_MHPMEVENT4,
        .name = "l1_icache_miss",
    },
    {
        .index = IUTLB_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER5,
        .event = CSR_MHPMEVENT5,
        .name = "I-uTLB miss",
    },
    {
        .index = DUTLB_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER6,
        .event = CSR_MHPMEVENT6,
        .name = "D-uTLB miss",
    },
    {
        .index = JTLB_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER7,
        .event = CSR_MHPMEVENT7,
        .name = "J-uTLB miss",
    },
    {
        .index = CONDITIONAL_BRANCH_MISPREDICT_COUNTER,
        .count = CSR_MHPMCOUNTER8,
        .event = CSR_MHPMEVENT8,
        .name = "conditional branch mispredict",
    },
    {
        .index = CONDITIONAL_BRANCH_INSTRUCTION_COUNTER,
        .count = CSR_MHPMCOUNTER9,
        .event = CSR_MHPMEVENT9,
        .name = "conditional branch inst",
    },
    {
        .index = STORE_INST_COUNTER,
        .count = CSR_MHPMCOUNTER10,
        .event = CSR_MHPMEVENT10,
        .name = "store inst",
    },
};

static struct event_counter_table table_2[] =
{
    {
        .index = L1_DCACHE_READ_ACCESS_COUNTER,
        .count = CSR_MHPMCOUNTER3,
        .event = CSR_MHPMEVENT3,
        .name = "l1 dcache read access",
    },
    {
        .index = L1_DCACHE_READ_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER4,
        .event = CSR_MHPMEVENT4,
        .name = "l1 dcache read miss",
    },
    {
        .index = L1_DCACHE_WRITE_ACCESS_COUNTER,
        .count = CSR_MHPMCOUNTER5,
        .event = CSR_MHPMEVENT5,
        .name = "l1 dcache write access",
    },
    {
        .index = L1_DCACHE_WRITE_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER6,
        .event = CSR_MHPMEVENT6,
        .name = "l1 dcache write miss",
    },
    {
        .index = ALU_INST_COUNTER,
        .count = CSR_MHPMCOUNTER7,
        .event = CSR_MHPMEVENT7,
        .name = "alu inst",
    },
    {
        .index = LOAD_STORE_INST_COUNTER,
        .count = CSR_MHPMCOUNTER8,
        .event = CSR_MHPMEVENT8,
        .name = "load store inst",
    },
    {
        .index = VECTOR_INST_COUNTER,
        .count = CSR_MHPMCOUNTER9,
        .event = CSR_MHPMEVENT9,
        .name = "vector inst",
    },
    {
        .index = CSR_ACCESS_INST_COUNTER,
        .count = CSR_MHPMCOUNTER10,
        .event = CSR_MHPMEVENT10,
        .name = "csr access inst",
    },
};

static struct event_counter_table table_3[] =
{
    {
        .index = SYNC_INST_COUNTER,
        .count = CSR_MHPMCOUNTER3,
        .event = CSR_MHPMEVENT3,
        .name = "sync inst",
    },
    {
        .index = LOAD_STORE_UNALIGN_ACCESS_INST_COUNTER,
        .count = CSR_MHPMCOUNTER4,
        .event = CSR_MHPMEVENT4,
        .name = "load store unalign access inst",
    },
    {
        .index = INTERRUPT_COUNTER,
        .count = CSR_MHPMCOUNTER5,
        .event = CSR_MHPMEVENT5,
        .name = "interrupt number",
    },
    {
        .index = INTERRUPT_OFF_CYCLE_COUNTER,
        .count = CSR_MHPMCOUNTER6,
        .event = CSR_MHPMEVENT6,
        .name = "interrupt off cycle",
    },
    {
        .index = ECALL_INST_COUNTER,
        .count = CSR_MHPMCOUNTER7,
        .event = CSR_MHPMEVENT7,
        .name = "ecall inst",
    },
    {
        .index = LONG_JUMP_INST_COUNTER,
        .count = CSR_MHPMCOUNTER8,
        .event = CSR_MHPMEVENT8,
        .name = "long jump inst",
    },
    {
        .index = FRONTEND_STALLED_CYCLE_COUNTER,
        .count = CSR_MHPMCOUNTER9,
        .event = CSR_MHPMEVENT9,
        .name = "frontend stall cycle inst",
    },
    {
        .index = FLOAT_POINT_INST_COUNTER,
        .count = CSR_MHPMCOUNTER10,
        .event = CSR_MHPMEVENT10,
        .name = "float point inst",
    },
};

static struct event_counter_table *get_table(int index, int *size)
{
    if (index == 0)
    {
        *size = ARRAY_SIZE(table_1);
        return &table_1;
    }
    else if (index == 1)
    {
        *size = ARRAY_SIZE(table_2);
        return &table_2;
    }
    else if (index == 2)
    {
        *size = ARRAY_SIZE(table_3);
        return &table_3;
    }
    return NULL;
}
#else
static struct event_counter_table table_1[] =
{
	{
        .index = L1_ICACHE_ACCESS_COUNTER,
        .count = CSR_MHPMCOUNTER3,
        .event = CSR_MHPMEVENT3,
        .name = "l1_icache_access",
    },
    {
        .index = L1_ICACHE_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER4,
        .event = CSR_MHPMEVENT4,
        .name = "l1_icache_miss",
    },
    {
        .index = L1_DCACHE_READ_ACCESS_COUNTER,
        .count = CSR_MHPMCOUNTER14,
        .event = CSR_MHPMEVENT14,
        .name = "l1 dcache read access",
    },
    {
        .index = L1_DCACHE_READ_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER15,
        .event = CSR_MHPMEVENT15,
        .name = "l1 dcache read miss",
    },
    {
        .index = L1_DCACHE_WRITE_ACCESS_COUNTER,
        .count = CSR_MHPMCOUNTER16,
        .event = CSR_MHPMEVENT16,
        .name = "l1 dcache write access",
    },
    {
        .index = L1_DCACHE_WRITE_MISS_COUNTER,
        .count = CSR_MHPMCOUNTER17,
        .event = CSR_MHPMEVENT17,
        .name = "l1 dcache write miss",
    },
};

static struct event_counter_table *get_table(int index, int *size)
{
    if (index == 0)
    {
        *size = ARRAY_SIZE(table_1);
        return &table_1;
    }
    return NULL;
}

#endif

static void hpm_exit(void)
{
    csr_write(mcountinhibit, 0xffffffff);
    csr_write(mcycle, 0);
    csr_write(minstret, 0);
    csr_write(mhpmcounter3, 0);
    csr_write(mhpmcounter4, 0);
    csr_write(mhpmcounter5, 0);
    csr_write(mhpmcounter6, 0);
    csr_write(mhpmcounter7, 0);
    csr_write(mhpmcounter8, 0);
    csr_write(mhpmcounter9, 0);
    csr_write(mhpmcounter10, 0);
}

static void set_event_value(int event, int index)
{
    if (event == CSR_MHPMEVENT3)
    {
        csr_write(mhpmevent3, index);
    }
    if (event == CSR_MHPMEVENT4)
    {
        csr_write(mhpmevent4, index);
    }
    if (event == CSR_MHPMEVENT5)
    {
        csr_write(mhpmevent5, index);
    }
    if (event == CSR_MHPMEVENT6)
    {
        csr_write(mhpmevent6, index);
    }
    if (event == CSR_MHPMEVENT7)
    {
        csr_write(mhpmevent7, index);
    }
    if (event == CSR_MHPMEVENT7)
    {
        csr_write(mhpmevent7, index);
    }
    if (event == CSR_MHPMEVENT8)
    {
        csr_write(mhpmevent8, index);
    }
    if (event == CSR_MHPMEVENT9)
    {
        csr_write(mhpmevent9, index);
    }
    if (event == CSR_MHPMEVENT10)
    {
        csr_write(mhpmevent10, index);
    }
    if (event == CSR_MHPMEVENT14)
    {
        csr_write(mhpmevent14, index);
    }
    if (event == CSR_MHPMEVENT15)
    {
        csr_write(mhpmevent15, index);
    }
    if (event == CSR_MHPMEVENT16)
    {
        csr_write(mhpmevent16, index);
    }
    if (event == CSR_MHPMEVENT17)
    {
        csr_write(mhpmevent17, index);
    }
}

static void set_counter_value(int counter, int value)
{
    if (counter == CSR_MHPMCOUNTER3)
    {
        csr_write(mhpmcounter3, value);
    }
    if (counter == CSR_MHPMCOUNTER4)
    {
        csr_write(mhpmcounter4, value);
    }
    if (counter == CSR_MHPMCOUNTER5)
    {
        csr_write(mhpmcounter5, value);
    }
    if (counter == CSR_MHPMCOUNTER6)
    {
        csr_write(mhpmcounter6, value);
    }
    if (counter == CSR_MHPMCOUNTER7)
    {
        csr_write(mhpmcounter7, value);
    }
    if (counter == CSR_MHPMCOUNTER8)
    {
        csr_write(mhpmcounter8, value);
    }
    if (counter == CSR_MHPMCOUNTER9)
    {
        csr_write(mhpmcounter9, value);
    }
    if (counter == CSR_MHPMCOUNTER14)
    {
        csr_write(mhpmcounter14, value);
    }
    if (counter == CSR_MHPMCOUNTER15)
    {
        csr_write(mhpmcounter15, value);
    }
    if (counter == CSR_MHPMCOUNTER16)
    {
        csr_write(mhpmcounter16, value);
    }
    if (counter == CSR_MHPMCOUNTER17)
    {
        csr_write(mhpmcounter17, value);
    }
}

static unsigned long get_counter_value(int count)
{
    if (count == CSR_MHPMCOUNTER3)
    {
        return csr_read(mhpmcounter3);
    }
    if (count == CSR_MHPMCOUNTER4)
    {
        return csr_read(mhpmcounter4);
    }
    if (count == CSR_MHPMCOUNTER5)
    {
        return csr_read(mhpmcounter5);
    }
    if (count == CSR_MHPMCOUNTER6)
    {
        return csr_read(mhpmcounter6);
    }
    if (count == CSR_MHPMCOUNTER7)
    {
        return csr_read(mhpmcounter7);
    }
    if (count == CSR_MHPMCOUNTER8)
    {
        return csr_read(mhpmcounter8);
    }
    if (count == CSR_MHPMCOUNTER9)
    {
        return csr_read(mhpmcounter9);
    }
    if (count == CSR_MHPMCOUNTER10)
    {
        return csr_read(mhpmcounter10);
    }
    if (count == CSR_MHPMCOUNTER14)
    {
        return csr_read(mhpmcounter14);
    }
    if (count == CSR_MHPMCOUNTER15)
    {
        return csr_read(mhpmcounter15);
    }
    if (count == CSR_MHPMCOUNTER16)
    {
        return csr_read(mhpmcounter16);
    }
    if (count == CSR_MHPMCOUNTER17)
    {
        return csr_read(mhpmcounter17);
    }
    return 0UL;
}

static void dump_hpmevent_registers(void)
{
    printf("mhpmevent3=0x%lx\n", csr_read(mhpmevent3));
    printf("mhpmevent4=0x%lx\n", csr_read(mhpmevent4));
    printf("mhpmevent5=0x%lx\n", csr_read(mhpmevent5));
    printf("mhpmevent6=0x%lx\n", csr_read(mhpmevent6));
    printf("mhpmevent7=0x%lx\n", csr_read(mhpmevent7));
    printf("mhpmevent8=0x%lx\n", csr_read(mhpmevent8));
    printf("mhpmevent9=0x%lx\n", csr_read(mhpmevent9));
    printf("mhpmevent10=0x%lx\n", csr_read(mhpmevent10));
    printf("mhpmevent13=0x%lx\n", csr_read(mhpmevent13));
    printf("mhpmevent14=0x%lx\n", csr_read(mhpmevent14));
    printf("mhpmevent15=0x%lx\n", csr_read(mhpmevent15));
    printf("mhpmevent16=0x%lx\n", csr_read(mhpmevent16));
    printf("mhpmevent17=0x%lx\n", csr_read(mhpmevent17));
    //printf("mhpmcr=0x%lx\n", csr_read(mhpmcr));
}

static void dump_hpmcount_registers(void)
{
    printf("mhpmcounter3=%ld\n", csr_read(mhpmcounter3));
    printf("mhpmcounter4=%ld\n", csr_read(mhpmcounter4));
    printf("mhpmcounter5=%ld\n", csr_read(mhpmcounter5));
    printf("mhpmcounter6=%ld\n", csr_read(mhpmcounter6));
    printf("mhpmcounter7=%ld\n", csr_read(mhpmcounter7));
    printf("mhpmcounter8=%ld\n", csr_read(mhpmcounter8));
    printf("mhpmcounter9=%ld\n", csr_read(mhpmcounter9));
    printf("mhpmcounter10=%ld\n", csr_read(mhpmcounter10));
    printf("mhpmcounter13=%ld\n", csr_read(mhpmcounter13));
    printf("mhpmcounter14=%ld\n", csr_read(mhpmcounter14));
    printf("mhpmcounter15=%ld\n", csr_read(mhpmcounter15));
    printf("mhpmcounter16=%ld\n", csr_read(mhpmcounter16));
    printf("mhpmcounter17=%ld\n", csr_read(mhpmcounter17));
}

static void hpm_reset(struct event_counter_table *table, int size)
{
    int i;

    csr_write(mcountinhibit, 0xffffffff);
    csr_write(mcycle, 0);
    csr_write(minstret, 0);

    for (i = 0; i < size; i++)
    {
        set_counter_value(table[i].count, 0);
        set_event_value(table[i].event, table[i].index);
    }

    csr_write(mcounteren, 0xffffffff);
    csr_write(scounteren, 0xffffffff);
    csr_write(mcountinhibit, 0);
}

static int dump_riscv_pmu_info(struct event_counter_table *table, int size)
{
    int i;

    printf("\n");
    printf("    ---------------PMU Report------------\n");
    printf("\n");

    for (i = 0; i < size; i++)
    {
        printf("    %s: %ld\n", table[i].name, get_counter_value(table[i].count));
    }

    printf("\n");
    printf("    -------------------------------------\n");
}

static void monitor_start(int index)
{
    struct event_counter_table *table = NULL;
    int size = 0;

    table = get_table(index, &size);

    dump_riscv_pmu_info(table, size);
    hpm_reset(table, size);
}

static void monitor_init(int index)
{
    struct event_counter_table *table = NULL;
    int size = 0;

    table = get_table(index, &size);

    hpm_reset(table, size);
}

static void pmu_thread_entry(void *param)
{
    monitor_init(nEvent);
    while (1)
    {
        /* printf("\e[1;1H\e[2J"); */
        monitor_start(nEvent);
        sleep(gDelay);
        if (exit_flag == 1)
        {
            exit_flag = 0;
            hpm_exit();
            break;
        }
    }
    pthread_exit(NULL);
}

int cmd_riscv_pmu(int argc, char **argv)
{
    int ret;
    int c = 0;
    pthread_attr_t attr;
    struct sched_param sched;
    pthread_t tid;

    optind = 0;
    gDelay = 3;
    nEvent = 0;

    while ((c = getopt(argc, argv, "d:n:h")) != -1)
    {
        switch (c)
        {
            case 'd':
                gDelay = atoi(optarg);
                break;
            case 'n':
                nEvent = atoi(optarg);
                break;
            case 'h':
                printf("Usage:riscv_pmu [-d delay_num] [-n number]\n");
                printf("      riscv_pmu -d 3 -n 0\n");
                return 0;
            default:
                break;
        }
    }

    sched.sched_priority = 20;
    pthread_attr_init(&attr);
    pthread_attr_setschedparam(&attr, &sched);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_attr_setstacksize(&attr, 4096);

    ret = pthread_create(&tid, &attr, pmu_thread_entry, NULL);
    if (ret != 0)
    {
        return -1;
    }
    pthread_setname_np(tid, "rv-pmu");

    while (1)
    {
        char cRxed = 0;

        cRxed = getchar();
        if (cRxed == 'q' || cRxed == 3)
        {
            exit_flag = 1;
            return 0;
        }
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_riscv_pmu, riscv_pmu, pmu);
