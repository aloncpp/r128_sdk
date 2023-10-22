#include <stdio.h>
#include <hal_cmd.h>
#include <hal_interrupt.h>
#include <csr.h>

typedef struct
{
    unsigned long pgd;
} pgd_t;

typedef struct
{
    unsigned long pte;
} pte_t;

typedef struct
{
    unsigned long pgprot;
} pgprot_t;

#define PAGE_SHIFT		12
#define PAGE_SIZE (4096)
#define PTRS_PER_PGD    (PAGE_SIZE / sizeof(pgd_t))

#define PGDIR_SHIFT     30
#define PGDIR_SIZE      (1 << PGDIR_SHIFT)

#define PFN_DOWN(x) ((x) >> PAGE_SHIFT)

#define __pgprot(x) ((pgprot_t) { (x) })
#define _PAGE_BASE  (_PAGE_PRESENT | _PAGE_ACCESSED | _PAGE_USER | \
                     _PAGE_SHARE | _PAGE_CACHE | _PAGE_BUF)

#define PAGE_NONE       __pgprot(_PAGE_PROT_NONE | _PAGE_CACHE | \
                                 _PAGE_BUF | _PAGE_SHARE | _PAGE_SHARE)
#define PAGE_READ       __pgprot(_PAGE_BASE | _PAGE_READ)
#define PAGE_WRITE      __pgprot(_PAGE_BASE | _PAGE_READ | _PAGE_WRITE)
#define PAGE_EXEC       __pgprot(_PAGE_BASE | _PAGE_EXEC)
#define PAGE_READ_EXEC      __pgprot(_PAGE_BASE | _PAGE_READ | _PAGE_EXEC)
#define PAGE_WRITE_EXEC     __pgprot(_PAGE_BASE | _PAGE_READ |  \
                                     _PAGE_EXEC | _PAGE_WRITE)

#define PAGE_COPY       PAGE_READ
#define PAGE_COPY_EXEC      PAGE_EXEC
#define PAGE_COPY_READ_EXEC PAGE_READ_EXEC
#define PAGE_SHARED     PAGE_WRITE
#define PAGE_SHARED_EXEC    PAGE_WRITE_EXEC

#define _PAGE_KERNEL        (_PAGE_READ \
                             | _PAGE_WRITE \
                             | _PAGE_PRESENT \
                             | _PAGE_ACCESSED \
                             | _PAGE_DIRTY \
                             | _PAGE_CACHE \
                             | _PAGE_SHARE \
                             | _PAGE_BUF   \
                             | _PAGE_GLOBAL)

#define PAGE_KERNEL     __pgprot(_PAGE_KERNEL)
#define PAGE_KERNEL_EXEC    __pgprot(_PAGE_KERNEL | _PAGE_EXEC)
#define PAGE_KERNEL_SO      __pgprot((_PAGE_KERNEL | _PAGE_SO) & \
                                     ~(_PAGE_CACHE | _PAGE_BUF))

#define PAGE_TABLE      __pgprot(_PAGE_TABLE)

#define _PAGE_ACCESSED_OFFSET 6

#define _PAGE_PRESENT   (1 << 0)
#define _PAGE_READ      (1 << 1)    /* Readable */
#define _PAGE_WRITE     (1 << 2)    /* Writable */
#define _PAGE_EXEC      (1 << 3)    /* Executable */
#define _PAGE_USER      (1 << 4)    /* User */
#define _PAGE_GLOBAL    (1 << 5)    /* Global */
#define _PAGE_ACCESSED  (1 << 6)    /* Set by hardware on any access */
#define _PAGE_DIRTY     (1 << 7)    /* Set by hardware on any write */
#define _PAGE_SOFT      (1 << 8)    /* Reserved for software */

/* C-SKY extend */
#define _PAGE_SEC   (1UL << 59)   /* Security */
#define _PAGE_SHARE (1UL << 60)   /* Shareable */
#define _PAGE_BUF   (1UL << 61)   /* Bufferable */
#define _PAGE_CACHE (1UL << 62)   /* Cacheable */
#define _PAGE_SO    (1UL << 63)   /* Strong Order */

#define _PAGE_SPECIAL   _PAGE_SOFT
#define _PAGE_TABLE     _PAGE_PRESENT

#define pgd_index(addr) (((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))
#define __pgd(x)    ((pgd_t) { (x) })
#define __pgprot(x) ((pgprot_t) { (x) })
#define pgd_val(x)  ((x).pgd)
#define pgprot_val(x)   ((x).pgprot)
#define _PAGE_PFN_SHIFT 10

#define MSTATUS_UIE	0x00000001
#define MSTATUS_SIE	0x00000002
#define MSTATUS_HIE	0x00000004
#define MSTATUS_MIE	0x00000008
#define MSTATUS_UPIE	0x00000010
#define MSTATUS_SPIE	0x00000020
#define MSTATUS_HPIE	0x00000040
#define MSTATUS_MPIE	0x00000080
#define MSTATUS_SPP	0x00000100
#define MSTATUS_HPP	0x00000600
#define MSTATUS_MPP	0x00001800
#define MSTATUS_FS	0x00006000
#define MSTATUS_XS	0x00018000
#define MSTATUS_MPRV	0x00020000
#define MSTATUS_PUM	0x00040000
#define MSTATUS_VM	0x1F000000
#define MSTATUS32_SD	0x80000000
#define MSTATUS64_SD	0x8000000000000000

#define MCAUSE32_CAUSE	0x7FFFFFFF
#define MCAUSE64_CAUSE	0x7FFFFFFFFFFFFFFF
#define MCAUSE32_INT	0x80000000
#define MCAUSE64_INT	0x8000000000000000

#define SSTATUS_UIE	0x00000001
#define SSTATUS_SIE	0x00000002
#define SSTATUS_UPIE	0x00000010
#define SSTATUS_SPIE	0x00000020
#define SSTATUS_SPP	0x00000100
#define SSTATUS_FS	0x00006000
#define SSTATUS_XS	0x00018000
#define SSTATUS_PUM	0x00040000
#define SSTATUS32_SD	0x80000000
#define SSTATUS64_SD	0x8000000000000000

#define BIT(nr)		(1 << (nr))

#define MIP_SSIP	BIT(IRQ_S_SOFT)
#define MIP_MSIP	BIT(IRQ_M_SOFT)
#define MIP_STIP	BIT(IRQ_S_TIMER)
#define MIP_MTIP	BIT(IRQ_M_TIMER)
#define MIP_SEIP	BIT(IRQ_S_EXT)
#define MIP_MEIP	BIT(IRQ_M_EXT)

#define SIP_SSIP	MIP_SSIP
#define SIP_STIP	MIP_STIP

#define PRV_U	0
#define PRV_S	1
#define PRV_H	2
#define PRV_M	3

#define VM_MBARE	0
#define VM_MBB		1
#define VM_MBBID	2

#define CSR_PMPCFG0			0x3a0
#define CSR_PMPCFG1			0x3a1
#define CSR_PMPCFG2			0x3a2
#define CSR_PMPCFG3			0x3a3
#define CSR_PMPADDR0			0x3b0
#define CSR_PMPADDR1			0x3b1
#define CSR_PMPADDR2			0x3b2
#define CSR_PMPADDR3			0x3b3
#define CSR_PMPADDR4			0x3b4
#define CSR_PMPADDR5			0x3b5
#define CSR_PMPADDR6			0x3b6
#define CSR_PMPADDR7			0x3b7
#define CSR_PMPADDR8			0x3b8
#define CSR_PMPADDR9			0x3b9
#define CSR_PMPADDR10			0x3ba
#define CSR_PMPADDR11			0x3bb
#define CSR_PMPADDR12			0x3bc
#define CSR_PMPADDR13			0x3bd
#define CSR_PMPADDR14			0x3be
#define CSR_PMPADDR15			0x3bf

static pgd_t system_pg_dir[PTRS_PER_PGD] __aligned(PAGE_SIZE);

static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
    unsigned long prot = pgprot_val(_prot);

    prot &= ~(_PAGE_CACHE | _PAGE_BUF);
    prot |= _PAGE_SO;

    return __pgprot(prot);
}

static inline pgd_t pfn_pgd(unsigned long pfn, pgprot_t prot)
{
    return __pgd((pfn << _PAGE_PFN_SHIFT) | pgprot_val(prot));
}

static void mmu_create_map_region(pgd_t *pgdir, unsigned long va, unsigned long pa, unsigned long size, pgprot_t prot)
{
    unsigned long pgd_index = pgd_index(va);

    if (size != PGDIR_SIZE)
    {
        printf("direct map area must be 1G size.\n");
        return;
    }

    if (pgd_val(pgdir[pgd_index]) == 0)
    {
        pgdir[pgd_index] = pfn_pgd(PFN_DOWN(pa), prot);
    }
    else
    {
        printf("already maped on pgdir index %ld.\n", pgd_index);
        return;
    }

    return;
}

static void clear_pgd(void)
{
    int i = 0;

    pgd_t nullpgd = {0};

    for (i = 0; i < PTRS_PER_PGD; i++)
    {
        system_pg_dir[i] = nullpgd;
    }
}

static void mmu_init(void)
{
    clear_pgd();

    mmu_create_map_region(system_pg_dir, 0UL, 0UL, \
                          PGDIR_SIZE, \
                          PAGE_KERNEL_EXEC);
    mmu_create_map_region(system_pg_dir, 0x40000000, \
                          0x40000000, \
                          0x40000000, \
                          pgprot_noncached(PAGE_KERNEL));

    unsigned long page_table_addr;

    page_table_addr = (unsigned long)&system_pg_dir;
    asm volatile("sfence.vma");
    csr_write(satp, (page_table_addr >> 12) | (8UL << 60));
    asm volatile("sfence.vma");
    return;
}

static void show_c906_mmu(void)
{
    printf("%s, %d\n", __func__, __LINE__);
}

static void s_mode_entry(void)
{
    int flag = 0;

    printf("init c906 mmu start\n");
    mmu_init();
    printf("init c906 mmu end  \n");

    while(1) {
        if (flag == 1) {
            show_c906_mmu();
        }
        printf("check flag failed-----\n");
    }
}

unsigned long csr_read_num(int csr_num)
{
	unsigned long ret = 0;

	switch (csr_num) {
	case CSR_PMPCFG0:
		ret = csr_read(CSR_PMPCFG0);
		break;
	case CSR_PMPCFG1:
		ret = csr_read(CSR_PMPCFG1);
		break;
	case CSR_PMPCFG2:
		ret = csr_read(CSR_PMPCFG2);
		break;
	case CSR_PMPCFG3:
		ret = csr_read(CSR_PMPCFG3);
		break;
	case CSR_PMPADDR0:
		ret = csr_read(CSR_PMPADDR0);
		break;
	case CSR_PMPADDR1:
		ret = csr_read(CSR_PMPADDR1);
		break;
	case CSR_PMPADDR2:
		ret = csr_read(CSR_PMPADDR2);
		break;
	case CSR_PMPADDR3:
		ret = csr_read(CSR_PMPADDR3);
		break;
	case CSR_PMPADDR4:
		ret = csr_read(CSR_PMPADDR4);
		break;
	case CSR_PMPADDR5:
		ret = csr_read(CSR_PMPADDR5);
		break;
	case CSR_PMPADDR6:
		ret = csr_read(CSR_PMPADDR6);
		break;
	case CSR_PMPADDR7:
		ret = csr_read(CSR_PMPADDR7);
		break;
	case CSR_PMPADDR8:
		ret = csr_read(CSR_PMPADDR8);
		break;
	case CSR_PMPADDR9:
		ret = csr_read(CSR_PMPADDR9);
		break;
	case CSR_PMPADDR10:
		ret = csr_read(CSR_PMPADDR10);
		break;
	case CSR_PMPADDR11:
		ret = csr_read(CSR_PMPADDR11);
		break;
	case CSR_PMPADDR12:
		ret = csr_read(CSR_PMPADDR12);
		break;
	case CSR_PMPADDR13:
		ret = csr_read(CSR_PMPADDR13);
		break;
	case CSR_PMPADDR14:
		ret = csr_read(CSR_PMPADDR14);
		break;
	case CSR_PMPADDR15:
		ret = csr_read(CSR_PMPADDR15);
		break;
	default:
		break;
	};

	return ret;
}

void csr_write_num(int csr_num, unsigned long val)
{
	switch (csr_num) {
	case CSR_PMPCFG0:
		csr_write(CSR_PMPCFG0, val);
		break;
	case CSR_PMPCFG1:
		csr_write(CSR_PMPCFG1, val);
		break;
	case CSR_PMPCFG2:
		csr_write(CSR_PMPCFG2, val);
		break;
	case CSR_PMPCFG3:
		csr_write(CSR_PMPCFG3, val);
		break;
	case CSR_PMPADDR0:
		csr_write(CSR_PMPADDR0, val);
		break;
	case CSR_PMPADDR1:
		csr_write(CSR_PMPADDR1, val);
		break;
	case CSR_PMPADDR2:
		csr_write(CSR_PMPADDR2, val);
		break;
	case CSR_PMPADDR3:
		csr_write(CSR_PMPADDR3, val);
		break;
	case CSR_PMPADDR4:
		csr_write(CSR_PMPADDR4, val);
		break;
	case CSR_PMPADDR5:
		csr_write(CSR_PMPADDR5, val);
		break;
	case CSR_PMPADDR6:
		csr_write(CSR_PMPADDR6, val);
		break;
	case CSR_PMPADDR7:
		csr_write(CSR_PMPADDR7, val);
		break;
	case CSR_PMPADDR8:
		csr_write(CSR_PMPADDR8, val);
		break;
	case CSR_PMPADDR9:
		csr_write(CSR_PMPADDR9, val);
		break;
	case CSR_PMPADDR10:
		csr_write(CSR_PMPADDR10, val);
		break;
	case CSR_PMPADDR11:
		csr_write(CSR_PMPADDR11, val);
		break;
	case CSR_PMPADDR12:
		csr_write(CSR_PMPADDR12, val);
		break;
	case CSR_PMPADDR13:
		csr_write(CSR_PMPADDR13, val);
		break;
	case CSR_PMPADDR14:
		csr_write(CSR_PMPADDR14, val);
		break;
	case CSR_PMPADDR15:
		csr_write(CSR_PMPADDR15, val);
		break;
	default:
		break;
	};
}

static unsigned long ctz(unsigned long x)
{
	unsigned long ret = 0;

	while (!(x & 1UL)) {
		ret++;
		x = x >> 1;
	}

	return ret;
}

#define PMP_R				0x01
#define PMP_W				0x02
#define PMP_X				0x04
#define PMP_A				0x18
#define PMP_A_TOR			0x08
#define PMP_A_NA4			0x10
#define PMP_A_NAPOT			0x18
#define PMP_L				0x80

#define PMP_SHIFT			2
#define PMP_COUNT			16

int pmp_set(unsigned int n, unsigned long prot, unsigned long addr,
	    unsigned long log2len)
{
	int pmpcfg_csr, pmpcfg_shift, pmpaddr_csr;
	unsigned long cfgmask, pmpcfg;
	unsigned long addrmask, pmpaddr;

	/* check parameters */
	if (n >= PMP_COUNT || log2len > __riscv_xlen || log2len < PMP_SHIFT)
		return -1;

		/* calculate PMP register and offset */
#if __riscv_xlen == 32
	pmpcfg_csr   = CSR_PMPCFG0 + (n >> 2);
	pmpcfg_shift = (n & 3) << 3;
#elif __riscv_xlen == 64
	pmpcfg_csr   = (CSR_PMPCFG0 + (n >> 2)) & ~1;
	pmpcfg_shift = (n & 7) << 3;
#else
	pmpcfg_csr   = -1;
	pmpcfg_shift = -1;
#endif
	pmpaddr_csr = CSR_PMPADDR0 + n;
	if (pmpcfg_csr < 0 || pmpcfg_shift < 0)
		return -2;

	/* encode PMP config */
	prot |= (log2len == PMP_SHIFT) ? PMP_A_NA4 : PMP_A_NAPOT;
	cfgmask = ~(0xffUL << pmpcfg_shift);
	pmpcfg	= (csr_read_num(pmpcfg_csr) & cfgmask);
	pmpcfg |= ((prot << pmpcfg_shift) & ~cfgmask);

	/* encode PMP address */
	if (log2len == PMP_SHIFT) {
		pmpaddr = (addr >> PMP_SHIFT);
	} else {
		if (log2len == __riscv_xlen) {
			pmpaddr = -1UL;
		} else {
			addrmask = (1UL << (log2len - PMP_SHIFT)) - 1;
			pmpaddr	 = ((addr >> PMP_SHIFT) & ~addrmask);
			pmpaddr |= (addrmask >> 1);
		}
	}

	/* write csrs */
	csr_write_num(pmpaddr_csr, pmpaddr);
	csr_write_num(pmpcfg_csr, pmpcfg);

	return 0;
}

static unsigned long log2roundup(unsigned long x)
{
	unsigned long ret = 0;

	while (ret < __riscv_xlen) {
		if (x <= (1UL << ret))
			break;
		ret++;
	}

	return ret;
}

static int sunxi_pmp_region_info(unsigned int hartid, unsigned int index, unsigned long *prot,
				 unsigned long *addr, unsigned long *log2size)
{
	int ret = 0;

	switch (index) {
	case 0:
		// get around sbi area 64k.
		*prot	  = PMP_R | PMP_W | PMP_X;
		*addr	  = 0x00000000;
		*log2size = log2roundup(0xFFFFFFFF);
		break;
	default:
		ret = -1;
		break;
	};

	return ret;
}

static void init_pmp(void)
{
    int i;
    unsigned long prot = 0;
    unsigned long addr = 0;
    unsigned long log2size = 0;

    for(i = 0; i < PMP_COUNT; i++) {
		if (sunxi_pmp_region_info(0, i, &prot, &addr, &log2size))
			continue;
		pmp_set(i + 1, prot, addr, log2size);
	}
}

static void s_mode_trap_entry(void)
{
    asm volatile("j .":::"memory");
}

#define EXTRACT_FIELD(val, which) (((val) & (which)) / ((which) & ~((which)-1)))
#define INSERT_FIELD(val, which, fieldval) (((val) & ~(which)) | ((fieldval) * ((which) & ~((which)-1))))

void enter_supervisor_mode(void (*fn)(void))
{
    unsigned long msValue = csr_read(mstatus);
    unsigned long ssValue = csr_read(sstatus);
    msValue = INSERT_FIELD(msValue, MSTATUS_MPP, PRV_S);
    msValue = INSERT_FIELD(msValue, MSTATUS_MPIE, 0);
    msValue = INSERT_FIELD(msValue, MSTATUS_MIE, 0);
    ssValue = INSERT_FIELD(ssValue, SSTATUS_SIE, 0);
    ssValue = INSERT_FIELD(ssValue, SSTATUS_SPIE, 0);

    csr_clear(sie, SIP_STIP);

    unsigned long midValue = csr_read(mideleg);
    midValue = 0xFFFFFFFFFFFFFFFFUL;
    csr_write(mideleg, midValue);
    unsigned long medValue = csr_read(medeleg);
    medValue = 0xFFFFFFFFFFFFFFFFUL;
    csr_write(medeleg, medValue);

    extern void s_mode_trap_entry(void);
    unsigned long super_trap_entry = (unsigned long)&s_mode_trap_entry;
    csr_write(stvec, super_trap_entry);
    midValue = csr_read(mideleg);
    medValue = csr_read(medeleg);
    printf("mideleg = %lx\n", midValue);
    printf("medeleg = %lx\n", medValue);

    csr_write(mcause, 0);
    csr_write(scause, 0);
    csr_write(mtval, 0);
    csr_write(stval, 0);

    csr_write(mstatus, msValue);
    csr_write(sstatus, ssValue);
    csr_write(mepc, fn);

    init_pmp();
    asm volatile("mret":::"memory");
    while(1);
    __builtin_unreachable();
}

static int cmd_c906_mmu(int argc, char ** argv)
{
    unsigned long flags = hal_interrupt_disable_irqsave();
    enter_supervisor_mode(s_mode_entry);
    hal_interrupt_enable_irqrestore(flags);
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(cmd_c906_mmu, c906_mmu, boot dsp);
