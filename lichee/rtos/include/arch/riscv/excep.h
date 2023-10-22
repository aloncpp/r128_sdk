#ifndef  __EXPRISCV_INC__
#define  __EXPRISCV_INC__

#ifndef __ASSEMBLY__

#include <stdint.h>

typedef struct
{
    uint32_t f[32];
    uint32_t fcsr;
} riscv_f_ext_state_t;

typedef struct
{
    uint64_t f[32];
    uint32_t fcsr;
} riscv_d_ext_state_t;

typedef struct
{
    uint64_t vxl;
    uint64_t vxh;
} riscv_vector_vhl;

typedef struct
{
    riscv_vector_vhl v[32];
    uint64_t vstart;
    /* uint64_t vxsat; */
    /* uint64_t vxrm; */
    uint64_t vl;
    uint64_t vtype;
    /* uint64_t vlenb; */
} riscv_v_ext_state_t;

typedef struct
{
    uint64_t f[64] __attribute__((aligned(16)));
    uint32_t fcsr;
    /*
    ¦* Reserved for expansion of sigcontext structure.  Currently zeroed
    ¦* upon signal, and must be zero upon sigreturn.
    ¦*/
    uint32_t reserved[3];
} riscv_q_ext_state_t;

typedef struct
{
    riscv_d_ext_state_t fpustatus;
} fpu_context_t;

typedef struct
{
    riscv_v_ext_state_t vectorstatus;
} vector_context_t;

// zero, x2(sp), x3(tp) need not backup.
typedef struct
{
    unsigned long mepc;            // 0 * __SIZEOF_LONG__
    unsigned long x1;              // 1 * __SIZEOF_LONG__
    unsigned long x5;              // 5 * __SIZEOF_LONG__
    unsigned long x6;              // 6 * __SIZEOF_LONG__
    unsigned long x7;              // 7 * __SIZEOF_LONG__
    unsigned long x8;              // 8 * __SIZEOF_LONG__
    unsigned long x9;              // 9 * __SIZEOF_LONG__
    unsigned long x10;             //10 * __SIZEOF_LONG__
    unsigned long x11;             //11 * __SIZEOF_LONG__
    unsigned long x12;             //12 * __SIZEOF_LONG__
    unsigned long x13;             //13 * __SIZEOF_LONG__
    unsigned long x14;             //14 * __SIZEOF_LONG__
    unsigned long x15;             //15 * __SIZEOF_LONG__
    unsigned long x16;             //16 * __SIZEOF_LONG__
    unsigned long x17;             //17 * __SIZEOF_LONG__
    unsigned long x18;             //18 * __SIZEOF_LONG__
    unsigned long x19;             //19 * __SIZEOF_LONG__
    unsigned long x20;             //20 * __SIZEOF_LONG__
    unsigned long x21;             //21 * __SIZEOF_LONG__
    unsigned long x22;             //22 * __SIZEOF_LONG__
    unsigned long x23;             //23 * __SIZEOF_LONG__
    unsigned long x24;             //24 * __SIZEOF_LONG__
    unsigned long x25;             //25 * __SIZEOF_LONG__
    unsigned long x26;             //26 * __SIZEOF_LONG__
    unsigned long x27;             //27 * __SIZEOF_LONG__
    unsigned long x28;             //28 * __SIZEOF_LONG__
    unsigned long x29;             //29 * __SIZEOF_LONG__
    unsigned long x30;             //30 * __SIZEOF_LONG__
    unsigned long x31;             //31 * __SIZEOF_LONG__
    unsigned long mstatus;         //32 * __SIZEOF_LONG__
    unsigned long x2;              // 2 * __SIZEOF_LONG__
    unsigned long x3;              // 3 * __SIZEOF_LONG__
    unsigned long x4;              // 4 * __SIZEOF_LONG__
    unsigned long mscratch;        //33 * __SIZEOF_LONG__
} irq_regs_t;

#endif

#define FPU_CTX_F0_F0   0   /* offsetof(fpu_context_t, fpustatus.f[0])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F1_F0   8   /* offsetof(fpu_context_t, fpustatus.f[1])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F2_F0   16  /* offsetof(fpu_context_t, fpustatus.f[2])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F3_F0   24  /* offsetof(fpu_context_t, fpustatus.f[3])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F4_F0   32  /* offsetof(fpu_context_t, fpustatus.f[4])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F5_F0   40  /* offsetof(fpu_context_t, fpustatus.f[5])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F6_F0   48  /* offsetof(fpu_context_t, fpustatus.f[6])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F7_F0   56  /* offsetof(fpu_context_t, fpustatus.f[7])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F8_F0   64  /* offsetof(fpu_context_t, fpustatus.f[8])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F9_F0   72  /* offsetof(fpu_context_t, fpustatus.f[9])  - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F10_F0  80  /* offsetof(fpu_context_t, fpustatus.f[10]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F11_F0  88  /* offsetof(fpu_context_t, fpustatus.f[11]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F12_F0  96  /* offsetof(fpu_context_t, fpustatus.f[12]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F13_F0  104 /* offsetof(fpu_context_t, fpustatus.f[13]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F14_F0  112 /* offsetof(fpu_context_t, fpustatus.f[14]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F15_F0  120 /* offsetof(fpu_context_t, fpustatus.f[15]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F16_F0  128 /* offsetof(fpu_context_t, fpustatus.f[16]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F17_F0  136 /* offsetof(fpu_context_t, fpustatus.f[17]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F18_F0  144 /* offsetof(fpu_context_t, fpustatus.f[18]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F19_F0  152 /* offsetof(fpu_context_t, fpustatus.f[19]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F20_F0  160 /* offsetof(fpu_context_t, fpustatus.f[20]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F21_F0  168 /* offsetof(fpu_context_t, fpustatus.f[21]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F22_F0  176 /* offsetof(fpu_context_t, fpustatus.f[22]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F23_F0  184 /* offsetof(fpu_context_t, fpustatus.f[23]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F24_F0  192 /* offsetof(fpu_context_t, fpustatus.f[24]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F25_F0  200 /* offsetof(fpu_context_t, fpustatus.f[25]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F26_F0  208 /* offsetof(fpu_context_t, fpustatus.f[26]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F27_F0  216 /* offsetof(fpu_context_t, fpustatus.f[27]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F28_F0  224 /* offsetof(fpu_context_t, fpustatus.f[28]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F29_F0  232 /* offsetof(fpu_context_t, fpustatus.f[29]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F30_F0  240 /* offsetof(fpu_context_t, fpustatus.f[30]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_F31_F0  248 /* offsetof(fpu_context_t, fpustatus.f[31]) - offsetof(fpu_context_t, fpustatus.f[0]) */
#define FPU_CTX_FCSR_F0 256 /* offsetof(fpu_context_t, fpustatus.fcsr)  - offsetof(fpu_context_t, fpustatus.f[0]) */


#define VECTOR_CTX_V0_V0   0   /* offsetof(vector_context_t, vectorstatus.v[0])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V1_V0   16  /* offsetof(vector_context_t, vectorstatus.v[1])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V2_V0   32  /* offsetof(vector_context_t, vectorstatus.v[2])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V3_V0   48  /* offsetof(vector_context_t, vectorstatus.v[3])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V4_V0   64  /* offsetof(vector_context_t, vectorstatus.v[4])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V5_V0   80  /* offsetof(vector_context_t, vectorstatus.v[5])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V6_V0   96  /* offsetof(vector_context_t, vectorstatus.v[6])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V7_V0   112  /* offsetof(vector_context_t, vectorstatus.v[7])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V8_V0   128  /* offsetof(vector_context_t, vectorstatus.v[8])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V9_V0   144  /* offsetof(vector_context_t, vectorstatus.v[9])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V10_V0   160  /* offsetof(vector_context_t, vectorstatus.v[10])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V11_V0   176  /* offsetof(vector_context_t, vectorstatus.v[11])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V12_V0   192  /* offsetof(vector_context_t, vectorstatus.v[12])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V13_V0   208  /* offsetof(vector_context_t, vectorstatus.v[13])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V14_V0   224  /* offsetof(vector_context_t, vectorstatus.v[14])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V15_V0   240  /* offsetof(vector_context_t, vectorstatus.v[15])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V16_V0   256  /* offsetof(vector_context_t, vectorstatus.v[16])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V17_V0   272  /* offsetof(vector_context_t, vectorstatus.v[17])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V18_V0   288  /* offsetof(vector_context_t, vectorstatus.v[18])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V19_V0   304  /* offsetof(vector_context_t, vectorstatus.v[19])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V20_V0   320  /* offsetof(vector_context_t, vectorstatus.v[20])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V21_V0   336  /* offsetof(vector_context_t, vectorstatus.v[21])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V22_V0   352  /* offsetof(vector_context_t, vectorstatus.v[22])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V23_V0   368  /* offsetof(vector_context_t, vectorstatus.v[23])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V24_V0   384  /* offsetof(vector_context_t, vectorstatus.v[24])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V25_V0   400  /* offsetof(vector_context_t, vectorstatus.v[25])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V26_V0   416  /* offsetof(vector_context_t, vectorstatus.v[26])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V27_V0   432  /* offsetof(vector_context_t, vectorstatus.v[27])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V28_V0   448  /* offsetof(vector_context_t, vectorstatus.v[28])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V29_V0   464  /* offsetof(vector_context_t, vectorstatus.v[29])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V30_V0   480  /* offsetof(vector_context_t, vectorstatus.v[30])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_V31_V0   496  /* offsetof(vector_context_t, vectorstatus.v[31])  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_VSTART_V0   512 /* offsetof(vector_context_t, vectorstatus.vstart)  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_Vl_V0   520  /* offsetof(vector_context_t, vectorstatus.vl)  - offsetof(vector_context_t, vectorstatus.v[0]) */
#define VECTOR_CTX_VTYPE_V0   528  /* offsetof(vector_context_t, vectorstatus.vtype)  - offsetof(vector_context_t, vectorstatus.v[0]) */

#endif
