#ifndef _RISCV_PMU_H
#define _RISCV_PMU_H

typedef enum
{
    L1_ICACHE_ACCESS_COUNTER = 0x1,
    L1_ICACHE_MISS_COUNTER,
    IUTLB_MISS_COUNTER,
    DUTLB_MISS_COUNTER,
    JTLB_MISS_COUNTER,
    CONDITIONAL_BRANCH_MISPREDICT_COUNTER,
    CONDITIONAL_BRANCH_INSTRUCTION_COUNTER,
    STORE_INST_COUNTER = 0xb,
    L1_DCACHE_READ_ACCESS_COUNTER,
    L1_DCACHE_READ_MISS_COUNTER,
    L1_DCACHE_WRITE_ACCESS_COUNTER,
    L1_DCACHE_WRITE_MISS_COUNTER,
    ALU_INST_COUNTER = 0x1d,
    LOAD_STORE_INST_COUNTER,
    VECTOR_INST_COUNTER,
    CSR_ACCESS_INST_COUNTER,
    SYNC_INST_COUNTER,
    LOAD_STORE_UNALIGN_ACCESS_INST_COUNTER,
    INTERRUPT_COUNTER,
    INTERRUPT_OFF_CYCLE_COUNTER,
    ECALL_INST_COUNTER,
    LONG_JUMP_INST_COUNTER,
    FRONTEND_STALLED_CYCLE_COUNTER,
    BACKEND_STALLED_CYCLE_COUNTER,
    SYNC_STALLED_CYCLE_COUNTER,
    FLOAT_POINT_INST_COUNTER,
} RISCV_PMU_EVENT;

typedef struct
{
    unsigned long l1_icache_access;
    unsigned long l1_icache_miss;
    unsigned long i_uTLB;
    unsigned long d_uTLB;
    unsigned long j_TLB;
    unsigned long branch_mispredict;
    unsigned long branch_inst;
    unsigned long store_inst;
    unsigned long l1_dcache_read_access;
    unsigned long l1_dcache_read_miss;
    unsigned long l1_dcache_write_access;
    unsigned long l1_dcache_write_miss;

    unsigned long alu_inst;
    unsigned long load_store_inst;
    unsigned long vector_inst;
    unsigned long csr_access_inst;
    unsigned long sync_inst;
    unsigned long load_store_unalign_access_inst;
    unsigned long interrupt_num;
    unsigned long interrupt_off_cycle;
    unsigned long ecall_inst;
    unsigned long long_jump_inst;
    unsigned long frontend_stall_cycle;
    unsigned long backend_stall_cycle;
    unsigned long sync_stall_cycle;
    unsigned long float_point_inst;
} rv_pmu_t;

int get_riscv_pmu_info(rv_pmu_t *pmu);

#endif
