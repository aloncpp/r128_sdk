#include "cpu_asm.h"

.extern xPort_V7a_Und_Trap_Entry
.extern xPort_V7a_Iabt_Trap_Entry
.extern xPort_V7a_Dabt_Trap_Entry
.extern xPort_V7a_Hype_Trap_Entry
.extern xPort_V7a_FIQ_Trap_Entry
.extern xport_cpu_page_table_init
.extern xport_cpu1_cache_op_flush
.extern xport_cpu1_mmu_op_enable
.extern xport_cpu1_cache_op_enable
.extern cache_flush_invalide_for_firmware
.extern start_kernel
.extern ARMV7_IRQ_DISABLE
.extern ARMV7_ALL_CACHE_DISABLE
.extern ARMV7_NEON_VFP_ENABLE
.extern disable_strict_align_check
.extern arm_branch_prediction_enable
.extern hypervisor_handler
.extern fiq_handler
.extern data_abort
.extern gic_irq_handler
.extern FreeRTOS_IRQ_Handler
.extern xport_cpu_disable_cache
#ifdef CONFIG_SMP
.extern xSchedulerRunning
#endif
.extern bootup_lock
.extern sunxi_spc_set_to_ns
.extern main_gic_init
.extern gic_cpuif_init
.extern main_init_gic
.global Vectors

.macro xport_core_id reg
    MRC     P15, #0, \reg, C0, C0, #5
    AND     \reg, \reg, #3
.endm 

//czl.porting from melis-v3.0
.macro COMM_ERROR_Handler  Handler
    STMFD   sp!, {r0-r12}

    @ get the lr and sp of prev. mode
    MRS     r0, spsr
    MRS     r1, cpsr
    ORR     r0, r0, #F_Bit
    ORR     r0, r0, #I_Bit
    MSR     cpsr_c, r0
    MOV     r2, sp                      @ get spsr mode sp
    MOV     r3, lr                      @ get spsr mode lr
    MSR     cpsr_c, r1

    @ push sp,orig_lr,cpsr, sctl, actl, abt_lr.
    STMFD   sp!, {r2}
    STMFD   sp!, {r3}
    MRS     r0,  spsr
    STMFD   sp!, {r0}
    MRC     p15, 0, r0, c1, c0, 1 
    STMFD   sp!, {r0}
    MRC     p15, 0, r0, c1, c0, 0
    STMFD   sp!, {r0}

    MRC     p15, 0, r0, c2, c0, 0   @ read TTBR0.
    STMFD   sp!, {r0}
    MRC     p15, 0, r0, c2, c0, 1   @ read TTBR1.
    STMFD   sp!, {r0}

    STMFD   sp!, {lr}

    @ prepare parameter for c code
    MRC     p15, 0,  r4, c5, c0, 0      @ get DFSR, cause.
    STMFD   sp!, {r4}
    MRC     p15, 0,  r4, c5, c0, 1      @ get IFSR.
    STMFD   sp!, {r4}
    MRC     p15, 0,  r4, c6, c0, 0      @ get DFAR, badvadr.
    STMFD   sp!, {r4}
    MRC     p15, 0,  r4, c6, c0, 2      @ get IFAR, badvadr.
    STMFD   sp!, {r4}
    MRC     p15, 0, r2, c13, c0, 0      @ get FCSEID.
    STMFD   sp!, {r2}
    MOV     r0,  sp

    BL      \Handler                    @ jumpto specify procedure.

    @ keep stack balance.
    LDMFD   sp!, {r0-r4}                @ popup fcseid, ifar, dfar, ifsr, dfsr.
    LDMFD   sp!, {lr}
    LDMFD   sp!, {r0-r1}                @ scr, sctl
    LDMFD   sp!, {r0-r1}                @ ttbr0, ttbr1
    LDMFD   sp!, {r0-r2}                @ orig_cpsr, sp, lr.
    LDMFD   sp!, {r0-r12}
    MOVS    pc,  lr
.endm

.section .freertos_vector
Vectors:
    B       Reset_Handler
    B       Undefined_Instruction
    B       SWI_Handler
    B       Prefetch_Abort_Handler
    B       Data_Abort
    B       Hypervisor_Handler
    B       IRQ_Handler
    B       FIQ_Handler

Monitor_vectors:
    B       .                     @ Reset      - not used by Monitor
    B       .                     @ Undef      - not used by Monitor
    B       SMC_Handler           @ Switch     - NS<---->S
    B       .                     @ Prefetch   - can by used by Monitor
    B       .                     @ Data abort - can by used by Monitor
    B       .                     @ RESERVED
    B       .                     @ IRQ        - can by used by Monitor
    B       .                     @ FIQ        - can by used by Monitor

SMC_Handler:
    PUSH   {r0-r3}

    @ Which world have we come from
    @ ------------------------------
    MRC     p15, 0, r0, c1, c1, 0        @ Read Secure Configuration Register data
    TST     r0, #NS_BIT                  @ Is the NS bit set?
    PUSH    {r0}                         @ Save a copy of the SCR on entry
    BIC     r0, r0, #NS_BIT              @ Make sure the SCR.NS bit is now clear
    MCR     p15, 0, r0, c1, c1, 0        @ Write Secure Configuration Register data
    ISB

    LDREQ   r0, =s_stack_sp
    LDRNE   r0, =ns_stack_sp
    LDR     r2, [r0]

    LDREQ   r1, =ns_stack_sp
    LDRNE   r1, =s_stack_sp
    LDR     r3, [r1]

    @ r2  <-- save to
    @ r3  <-- restore from

    @ Save general purpose registers, SPSR, LR and SP
    @ ------------------------------------------------
    STMFD   r2!, {r4-r12}               @ Save r4 to r12
    MRS     r4, spsr                    @ Also get a copy of the SPSR_mon
    STMFD   r2!, {r4, lr}               @ Save original SPSR_mon and LR_mon

    CPS     #ARM_MODE_SVC               @ Switch into Supervisor mode temporarily
    MOV     r4, sp                      @ Temp for SP_svc
    STMFD   r2!, {r4, lr}               @ Save SP_svc and LR_svc
    STR     r2, [r0]                    @ Save updated pointer back, r0 and r2 now free

    @ Restore other world's registers, SPSR and LR
    @ ---------------------------------------------
    LDMFD   r3!, {r4, lr}               @ Restore SP_svc and LR_svc
    MOV     sp, r4                      @ Temp for SP_svc
    CPS     #ARM_MODE_MON               @ Switch back into Monitor mode

    LDMFD   r3!, {r0, lr}               @ Get SPSR_mon and LR_mon (return address)
    MSR     spsr_cxsf, r0               @ Restore SPSR_mon
    LDMFD   r3!, {r4-r12}               @ Restore registers r4 to r12

    STR     r3, [r1]                    @ Save updated pointer back, r1 and r3 now free

    @ Clear local monitor
    @ --------------------
    CLREX                               @ Not strictly required in this example, as not using LDREX/STREX
                                        @ However, architecturally should execute CLREX on a context switch

    @ Set SCR.NS bit correctly for world we're entering
    @ --------------------------------------------------
    POP     {r0}
    EOR     r0, r0, #NS_BIT              @ Toggle NS bit
    MCR     p15, 0, r0, c1, c1, 0        @ Write Secure Configuration Register data

    @ Now restore args (r0-r3)
    @ -------------------------
    POP     {r0-r3}

    @ Perform exception return
    @ -------------------------
    MOVS    pc, lr

Reset_Handler:
    CPS     #ARM_MODE_SVC
    BL       monitor_init
    CPS     #ARM_MODE_SVC
    LDR     sp,=_stack_start
    BL      sunxi_spc_set_to_ns
    NOP  
    BL       main_gic_init
    NOP
    BL       enable_vfp_neon
    NOP
    @SMC      #0
    B        security

Normal_Reset:
    @B       .
    SMC     #0

security:
    MOV     fp, #0
    CPS     #ARM_MODE_SVC

    @ disabal interrupt capability.
    MOV     r0,#F_Bit
    ORR     r0, r0, #I_Bit
    MRS     r1, cpsr
    ORR     r1, r1, r0
    MSR     cpsr_c, r1

    MRC     p15, 0, r0, c1, c0, 0       @ Read CP15 System Control register
    BIC     r0, r0, #(0x1 << 12)        @ Clear I, bit 12, to disable I Cache
    BIC     r0, r0, #(0x1 << 11)        @ Clear Z, bit 11, to disable branch prediction
    BIC     r0, r0, #(0x1 <<  2)        @ Clear C, bit  2, to disable D Cache
    BIC     r0, r0, #(0x1 <<  1)        @ Clear A, bit  1, to disable strict alignment fault checking
    BIC     r0, r0, #0x1                @ Clear M, bit  0, to disable MMU
    MCR     p15, 0, r0, c1, c0, 0       @ Write CP15 System Control register

    LDR     r0, =Vectors
    MCR     p15, 0, r0, c12, c0, 0	@ Write Secure or Non-secure Vector Base Address
    MRC     p15, 0, r0, c1, c0, 0       @ Read Control Register
    BIC     r0, r0, #(1 << 13)          @ Clear the V bit (bit 13)
    ORR     r0, r0, #(1 << 14)          @ Cache roundrobin Replacement(bit 14)
    MCR     p15, 0, r0, c1, c0, 0       @ Write Control Register

    @ set startup stack of svc mode.
    LDR     r2,=_stack_start
    MOV     sp, r2
    SUB     r2, r2,  #svc_stack_len

    MRS     r0, CPSR

    @ Set up the fast interrupt stack pointer
    BIC     r0, r0, #MODE_MSK
    ORR     r0, r0, #ARM_MODE_FIQ
    MSR     cpsr_c, r0
    BIC     r3, r2, #0x7
    MOV     sp, r3
    MOV     fp, #0
    SUB     r2, #fiq_stack_len

    @ Set up the normal interrupt stack pointer
    BIC     r0, r0, #MODE_MSK
    ORR     r0, r0, #ARM_MODE_IRQ
    MSR     CPSR_c, r0
    BIC     r3, r2, #0x7
    MOV     sp,r3
    @MOV     sp,#SRAM_STACK_IRQ0
    SUB     r2,r2,  #irq_stack_len

    @ Set up the abort mode stack pointer
    BIC     r0, r0, #MODE_MSK
    ORR     r0, r0, #ARM_MODE_ABT
    MSR     CPSR_c, r0
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #abt_stack_len

    @ Set up the undefined mode stack pointer
    BIC     r0, r0, #MODE_MSK
    ORR     r0, r0, #ARM_MODE_UND
    MSR     CPSR_c, r0
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #und_stack_len

    @ Set up the system mode stack pointer
    BIC     r0, r0, #MODE_MSK
    ORR     r0, r0, #ARM_MODE_SYS
    MSR     CPSR_c, r0
    BIC     r3, r2, #0x7
    MOV     sp, r3
    SUB     r2, r2, #user_stack_len

    @ Set up the monitor mode stack pointer
    BIC     r0, r0, #MODE_MSK
    ORR     r0, r0, #ARM_MODE_MON
    MSR     CPSR_c, r0
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #mon_stack_len

    LDR     R0, =_stack_end
    CMP     R2, R0
    BGT     stack_safe_cpu0

    @ deadloop for stackoverflow.
    B       .

stack_safe_cpu0:
    LDR     R0, =__cpu1_stack_store
    STR     R2, [R0]

    CPS     #ARM_MODE_SVC

    MOV     r0,#0
    ldr     r1,=__tbss_start
    ldr     r2,=__tbss_end
tbss_loop:
    cmp     r1,r2
    strlo   r0,[r1],#4
    blo     tbss_loop

    MOV     r0,#0
    ldr     r1,=__sbss_start
    ldr     r2,=__sbss_end
sbss_loop:
    cmp     r1,r2
    strlo   r0,[r1],#4
    blo     sbss_loop

    MOV     r0,#0
    ldr     r1,=__bss_start
    ldr     r2,=__bss_end
bss_loop:
    cmp     r1,r2
    strlo   r0,[r1],#4
    blo     bss_loop

    BL      disable_strict_align_check
    BL      __flush_branch_target_cache
    BL      __disable_branch_prediction

    DSB
    ISB
    DMB

    BL  __tlb_init

    @ Branch Prediction Enable.
    MOV     r1, #0
    MRC     p15, 0, r1, c1, c0, 0 @ Read Control Register configuration data.
    ORR     r1, r1, #(0x1 << 11) @ Global BP Enable bit.
    MCR     p15, 0, r1, c1, c0, 0 @ Write Control Register configuration dat

    DSB
    DMB
    ISB

    @BL      __enable_scu
    @MOV     R0, #0x0
    @MOV     R1, #0xf
    @BL      __secure_scu_invalidate
    BL      __joinsmp
    @BL      __enable_maintenance_broadcast
    @BL      __enable_cpu_alignment

    @ Enable D-side Prefetch
    MRC     p15, 0, r1, c1, c0, 1 @ Read Auxiliary Control Register
    ORR     r1, r1, #(0x1 <<2)    @ Enable D-side prefetch
    MCR     p15, 0, r1, c1, c0, 1 @ Write Auxiliary Control Register.

    @MRC     P15, 0, r0, c2, c0, 0  //read TTBR0.
    @MRC     P15, 0, r1, c2, c0, 1  //read TTBR1.

    @BL     xport_cpu1_cache_op_flush
    BL      xport_cpu1_mmu_op_enable
    BL      xport_cpu1_cache_op_enable

    @ MMU Enabled and Inst. In the Right position.
    @ It is now safe to enable caches and brach prediction.
    BL      arm_branch_prediction_enable
    BL	    ARMV7_NEON_VFP_ENABLE
    BL      ARMV7_FLUSH_CACHE_ALL

    @ temporily disable i/d cache for spinlock sync error operation, opened after the uboot stage optimilized.
    @BL      xport_cpu_dcache_disable
    @BL      xport_cpu_icache_disable
    @BL      xport_cpu_disable_cache

    DSB
    ISB
    DMB

#ifdef CONFIG_KASAN
    BL      kasan_early_init
#endif

    BL       __enable_branch_prediction
    NOP
    BL       start_kernel

    @ never be here.
cpu0_halt:
    B       .

__cpu1_stack_store:
   .word    0xffffffff

__enable_cpu_alignment:
    MRC     p15, 0, r1, c1, c0, 0
    ORR     r1, #(1<<1)
    MCR     p15, 0, r1, c1, c0, 0
    BX      lr

__user_setup_stackheap:
    MOV     r0,#0
    MOV     r2,#0
    BX	    lr

__enable_scu:
    MRC     p15, 4, r0, c15, c0, 0  @ Read periph base address
    LDR     r1, [r0, #0x0]          @ Read the SCU Control Register
    ORR     r1, r1, #0x1            @ Set bit 0 (The Enable bit)
    STR     r1, [r0, #0x0]          @ Write back modifed value
    BX      lr

__secure_scu_invalidate:
    AND     r0, r0, #0x03           @ Mask off unused bits of CPU ID
    MOV     r0, r0, LSL #2          @ Convert into bit offset (four bits per core)
    AND     r1, r1, #0x0F           @ Mask off unused bits of ways
    MOV     r1, r1, LSL r0          @ Shift ways into the correct CPU field
    MRC     p15, 4, r2, c15, c0, 0  @ Read periph base address
    STR     r1, [r2, #0x0C]         @ Write to SCU Invalidate All in Secure State
    BX      lr  

__enable_maintenance_broadcast:
    MRC     p15, 0, r0, c1, c0, 1   @ Read Aux Ctrl register
    MOV     r1, r0
    ORR     r0, r0, #0x01           @ Set the FW bit (bit 0)
    CMP     r0, r1
    MCRNE   p15, 0, r0, c1, c0, 1   @ Write Aux Ctrl register
    BX      lr

__tlb_init:
    @ Disable MMU
    MRC     p15, 0, r1, c1, c0, 0
    BIC     r1, r1, #0x1
    MCR     p15, 0, r1, c1, c0, 0

    @ Disable L1 Cache
    MRC     p15, 0, r1, c1, c0, 0
    BIC     r1, r1, #(0x1 << 12)     @Disable I Cache
    BIC     r1, r1, #(0x1 << 2)      @Disable D Cache
    MCR     p15, 0, r1, c1, c0, 0

    @ Invalidate Data Cache
    @ Invalidate Instruction Cache.
    MOV     r1, #0
    MCR     P15, 0, R1, C7, C5, 0

    @ Invalidate Data cache.
    @ To make the code general purpose, calculate the
    @ cache size first and loop through each set + way.

    MRC     p15, 1, r0, c0, c0, 0 @ Read Cache Size ID.
    MOV     r3, #0x1ff
    AND     r0, r3, r0, LSR #13 @ r0 = no. of sets - 1.

    MOV     r1, #0
way_loop_cpu:
    MOV     r3, #0
set_loop_cpu:
    MOV     r2, r1, LSL #30
    ORR     r2, r3, LSL #5 @ r2 = set/way cache operation format.
    MCR     p15, 0, r2, c7, c6, 2 @ Invalidate the line described by r2.
    ADD     r3, r3, #1 @ Increment set counter.
    CMP     r0, r3 @ Last set reached yet?
    BGT     set_loop_cpu @ If not, iterate set_loop,
    ADD     r1, r1, #1 @ else, next.
    CMP     r1, #4 @ Last way reached yet?
    BNE     way_loop_cpu @ if not, iterate way_loop_cpu

    @ Invalidate TLB
    MOV     r0, #0x0
    MCR     p15, 0, r0, c8, c7, 0      @ TLBIALL - Invalidate entire Unified TLB

    @ If N is set to 0, always use TTBR0. This is the default case at reset for the Secure version of this register. 
    @ It is backwards compatible with ARMv5 and earlier processors. 
    @ If N is set greater than 0, and bits [31:32-N] of the VA are all zeros, use TTBR0, 
    @ otherwise use TTBR1. N must be in the range 0-7.
    @ So set to zero means use TTBR0 only.
    MOV     r0,#0x0
    MCR     p15, 0, r0, c2, c0, 2

    BX      lr
#ifdef CONFIG_SMP
.global secondary_cpu_start
secondary_cpu_start:
    @switch to supersior mode.
    CPS     #ARM_MODE_SVC
    BL      cpu1_enable_gic_intf 

reload:
    LDR     R0,  =xSchedulerRunning
    MOV     R1,  #0          		@ first cpu scheduler flag.
    LDR     R0, [R0, R1, lsl #2]
    
    MOV     R1,  #1                     @ pdTRUE flag. 
    CMP     R0,  R1

    WFINE

    BNE     reload

    @ disable interrupt capability.
    MOV     r0,#F_Bit
    ORR     r0, r0, #I_Bit
    MRS     r1, cpsr
    ORR     r1, r1, r0
    MSR     cpsr_c, r1

    MRC     p15, 0, r0, c1, c0, 0
    BIC     r0, #(1<<13)
    MCR     p15, 0, r0, c1, c0, 0

    MRC     P15, 0, r0, c2, c0, 0   //read TTBR0.
    MRC     P15, 0, r1, c2, c0, 1   //read TTBR1.

    LDR     R2, __cpu1_stack_store

    CPS     #ARM_MODE_IRQ
    BIC     r3, r2, #0x7
    MOV     sp,r3
    @MOV     sp,#SRAM_STACK_IRQ1
    SUB     r2,r2,  #irq_stack_len

    CPS     #ARM_MODE_FIQ
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #fiq_stack_len

    CPS     #ARM_MODE_ABT
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #abt_stack_len

    CPS     #ARM_MODE_UND
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #und_stack_len

    CPS     #ARM_MODE_SVC
    BIC     r3, r2, #0x7
    MOV     sp,r3
    SUB     r2,r2,  #svc_stack_len

    LDR     R0, =_stack_end
    CMP     R2, R0
    BGT     stack_safe_cpu1

    @ deadloop if stack overflow.
    B       .

stack_safe_cpu1:
    NOP
    BL      __tlb_init
    NOP
    @ Branch Prediction Enable.
    MOV     r1, #0
    MRC     p15, 0, r1, c1, c0, 0       @ Read Control Register configuration data.
    ORR     r1, r1, #(0x1<<11)          @ Global BP Enable bit.
    BIC     r0, r0, #(0x1<<13)
    ORR     r0, r0, #(1 << 14)          @ Cache roundrobin Replacement(bit 14)
    MCR     p15, 0, r1, c1, c0, 0       @ Write Control Register configuration dat

    DSB
    DMB
    ISB

    @ C12-C0 is only active when SCTLR.V = 0
    MRC     p15, 0, r0, c1, c0, 0
    MCR     p15, 0, r0, c1, c0, 0

    @ Install exception handler, same as processor0.
    LDR     r0, =Vectors
    MCR     p15, 0, r0, c12, c0, 0

    MRC     p15, 0, r0, c0, c0, 5 
    ANDS    R0, R0, #0x03
    MOV     R1, #0xf
    @BL      __secure_scu_invalidate
    BL      __joinsmp 
    @BL      __enable_maintenance_broadcast
    @BL      __enable_cpu_alignment

    BL       enable_vfp_neon

    @ Enable D-side Prefetch
    MRC     p15, 0, r1, c1, c0, 1  @ Read Auxiliary Control Register
    ORR     r1, r1, #(0x1 <<2)     @ Enable D-side prefetch
    MCR     p15, 0, r1, c1, c0, 1  @ Write Auxiliary Control Register.

    BL     __set_generic_timer

    BL	    ARMV7_NEON_VFP_ENABLE
    @BL     xport_cpu1_cache_op_flush
    BL      xport_cpu1_mmu_op_enable
    BL      xport_cpu1_cache_op_enable

    @ temporily disable, need inverstigae the uboot mmap stage.
    @BL      xport_cpu_dcache_disable
    @BL      xport_cpu_icache_disable
    @BL      xport_cpu_disable_cache

    DSB     
    ISB
    DMB

    @BL      main_gic_init
    BL       gic_cpuif_init
    NOP
    BL       __enable_branch_prediction
    NOP
    BL       cpu1_c_main

    @never be here.
cpu1_halt:
    B       .

.global cpu1_enable_gic_intf
cpu1_enable_gic_intf:
    @ Basic GIC Interface INIT.
    MOV     R0,  #0x2000    
    MOVT    R0,  #0x0302
    MOV     R1,  #0xf
    STR     R1,  [R0]
    MOV     R1,  #0xff
    STR     R1,  [R0,#4]
    DSB
    DMB
    ISB
    BX      LR
#endif

@ Reserving some memory to save Normal world state information
.size ns_stack_base, 124
ns_stack_base:
    @.word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .zero 124
.size ns_stack_sp, 4
ns_stack_sp:
    .word 0

.size s_stack_base, 124
s_stack_base:
    .word 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    @.zero 124
.size s_stack_sp, 4
s_stack_sp:
    .word 0
@ ----HIGH----------------------------------------------------->LOW
@ r12, r11, r10, r09, r08, r07, r06, r05, r04, R03, R02, R01, R00
@ ---------------------------------------------mod--ent-svcsp-svclr
monitor_init:
    @ initialize monitor vector table.
    LDR     r0, =Monitor_vectors
    MCR     p15, 0, r0, c12, c0, 1

    MOV     r0, lr 
    BL     __set_generic_timer
    MOV     lr, r0

    @ Enable access to Coprocessors 10 and 11 from non-secure code too
    MRC     p15, 0, r0, c1, c1, 2 
    ORR     r0, r0, #(0x3 << 10)
    ORR     r0, r0, #(0x1 << 18)
    MCR     p15, 0, r0, c1, c1, 2
    ISB

    MRC     P15, 0, r1, c1, c0, 1
    MOV     r0, #(1<<6)
    ORR     r1, r0
    MCR     P15, 0, r1, c1, c0, 1  //enable smp

    MRC     p15, 0, r1, c1, c0, 0
    ORR     r1, #(1<<13)
    BIC     r1, #(1 << 0 | 1 << 2)
    BIC     r1, #(1 << 12)
    BIC     r1, #(1 << 30)
    MCR     p15, 0, r1, c1, c0, 0

    @ Set AW,FW in SCR register.
    MRC     p15, 0, r0, c1, c1, 0
    ORR     r0, r0, #(0x3 << 4)
    MCR     p15, 0, r0, c1, c1, 0
    ISB

    @ set startup stack of svc mode.
    @ reuse the svc stack here, and later 
    @ would re-init.
    CPS     #ARM_MODE_MON
    LDR     sp,=_stack_start
    CPS     #ARM_MODE_SVC

    LDR     r1, =ns_stack_base
    ADD     r1, r1, #124
    MOV     r2, #0
    MOV     r3, #0

    STMFD   r1!, {r2-r3}             @ Save off "dummy" values for r11-r12
    STMFD   r1!, {r2-r3}             @ Save off "dummy" values for r9-r10
    STMFD   r1!, {r2-r3}             @ Save off "dummy" values for r7-r8
    STMFD   r1!, {r2-r3}             @ Save off "dummy" values for r5-r6
    STMFD   r1!, {r2}                @ Save off "dummy" values for r4
   
    @ NOT saving value for r0-r3, as we are using these registers to pass information between worlds 
    MOV     r2,  #ARM_MODE_SVC
    LDR     r3,  =Normal_Reset
    STMFD   r1!, {r2,r3}
    MOV     r2,  #0xbead
    MOVT    r2,  #0xdead
    MOV     r3,  #0xbead
    MOVT    r3,  #0xdead
    STMFD   r1!, {r2,r3}

    LDR     r0, =ns_stack_sp
    STR     r1, [r0]

    LDR     r0, =s_stack_base
    ADD     r0, r0, #124
    LDR     r1, =s_stack_sp
    STR     r0, [r1]

    BX      lr

__enable_cpu1_gic:
    B       .
    MRC     p15, 4, r0, c15, c0, 0   @ Read periph base address
    LDR     r1, [r0, #0x100]         @ Read the Processor Interface Control register (ICCICR/ICPICR)
    ORR     r1, r1, #0x03            @ Bit 0: Enables secure interrupts, Bit 1: Enables Non-Secure interrupts
    STR     r1, [r0, #0x100]         @ Write the Processor Interface Control register (ICCICR/ICPICR)
    BX      lr

__set_generic_timer:
    @set cntfrq to 24M
    LDR     r1, =24000000
    MCR     p15, 0, r1, c14, c0, 0
    BX      lr 

__joinsmp:
    MRC     P15, 0, r1, c1, c0, 1
    MOV     r0, #(1<<6)
    ORR     r1, r0
    MCR     P15, 0, r1, c1, c0, 1  //enable smp
    BX      lr

__exitsmp:
    @ SMP status is controlled by bit 6 of the CP15 Aux Ctrl Reg
    MRC     p15, 0, r0, c1, c0, 1   @ Read ACTLR
    BIC     r0, r0, #0x040          @ Clear bit 6
    MCR     p15, 0, r0, c1, c0, 1   @ Write ACTLR
    BX      lr  

__enable_branch_prediction:
    MRC     p15, 0, r0, c1, c0, 0                 @ Read SCTLR
    ORR     r0, r0, #(1 << 11)                    @ Set the Z bit (bit 11)
    MCR     p15, 0,r0, c1, c0, 0                  @ Write SCTLR
    BX      lr

__flush_branch_target_cache:
    @ Invalidate entire branch predictor array
    MOV     r0, #0
    MCR     p15, 0, r0, c7, c5, 6 
    BX      lr

__disable_branch_prediction:
    MRC     p15, 0, r0, c1, c0, 0                 @ Read SCTLR
    BIC     r0, r0, #(1 << 11)                    @ Clear the Z bit (bit 11)
    MCR     p15, 0,r0, c1, c0, 0                  @ Write SCTLR
    BX      lr

.global xport_get_cpu_nums
xport_get_cpu_nums:
    @ Get base address of private peripheral space
    MRC     p15, 4, r0, c15, c0, 0  @ Read periph base address
    LDR     r0, [r0, #0x004]        @ Read SCU Configuration register
    AND     r0, r0, #0x3            @ Bits 1:0 gives the number of cores-1
    ADD     r0, r0, #1
    BX      lr

@ Debug MMU. CZL Add.
.global xport_set_ATS1CPR
xport_set_ATS1CPR:
    LDR    r0, =bootup_lock
    MOV    r2, r0
    MCR    p15,0,r0,c7,c8,0
    ISB
    mrrc   p15, 0, r0, r1, c7
    B      .
    BX     lr

//CZL add for sending IPI interrupt.
@ void send_ipi_interrupt(int ipino, unsigned int targetlist, unsigned int filter, bool nsattr);
.global send_ipi_interrupt
send_ipi_interrupt:
    AND    r0, r0, #0x0f
    AND    r1, r1, #0x0f
    AND    r2, r2, #0x0f

    ORR    r0, r0, r1,  LSL #16
    ORR    r0, r0, r2,  LSL #24

    CMP    r3, #0x0

    ORREQ  r0, #(0 << 15)     @ set nsatt
    ORRNE  r0, #(1 << 15)

    MOV    r1, #0x1000
    MOVT   r1, #0x0302
    ADD    r1, #0xf00      @SGI Trigger.

    DMB    ishst
    STR    r0, [r1]

    ISB	

    BX     lr

.global enbale_vfp_neon
enable_vfp_neon:
    MRC     p15, 0, r0, c1, c0, 2
    ORR     r0, r0, #(0xF << 20)
    MCR     p15, 0, r0, c1, c0, 2
    ISB

    FMRX    r0, FPEXC
    ORR     r0, r0, #0x40000000
    FMXR    FPEXC, r0
    BX      LR

Undefined_Instruction:
    COMM_ERROR_Handler  xPort_V7a_Und_Trap_Entry
Data_Abort:
    COMM_ERROR_Handler  xPort_V7a_Dabt_Trap_Entry
Hypervisor_Handler:
    COMM_ERROR_Handler  xPort_V7a_Hype_Trap_Entry
Prefetch_Abort_Handler:
    COMM_ERROR_Handler  xPort_V7a_Iabt_Trap_Entry

IRQ_Handler:
    B  FreeRTOS_IRQ_Handler

SWI_Handler:
    B  FreeRTOS_SWI_Handler

@ FIQ not supported!
FIQ_Handler:
    COMM_ERROR_Handler  xPort_V7a_FIQ_Trap_Entry
