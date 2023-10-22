#ifndef __MMU_I_H__
#define __MMU_I_H__

#define DESC_SEC       (0x2)
#define MEMWBWAYC      ((1<<12)|(3<<2))     /* write back, write allocate,cache enable */

#define MEMWB          (3<<2)  /* write back, no write allocate */
#define MEMWT          (2<<2)  /* write through, no write allocate */
#define SHAREDEVICE    (1<<2)  /* shared device */
#define STRONGORDER    (0<<2)  /* strong ordered */
#define XN             (1<<4)  /* eXecute Never */
#define AP_RW          (3<<10) /* supervisor=RW, user=RW */
#define AP_RO          (2<<10) /* supervisor=RW, user=RO */
#define SHARED         (1<<16) /* shareable */
#define NS             (1<<19) /* access in ns mode. */

#define DOMAIN_FAULT   (0x0)
#define DOMAIN_CHK     (0x1)
#define DOMAIN_NOTCHK  (0x3)
#define DOMAIN0        (0x0<<5)
#define DOMAIN1        (0x1<<5)
#define DOMAIN15       (0xf<<5)

#define DOMAIN0_ATTR   (DOMAIN_CHK<<0)
#define DOMAIN1_ATTR   (DOMAIN_FAULT<<2)

#define NORMAL_MEM_CACHED   (SHARED|AP_RW|DOMAIN0|MEMWBWAYC|DESC_SEC)
#define NORMAL_MEM_UNCACHED (SHARED|AP_RW|DOMAIN0|MEMWBWANC|DESC_SEC)
#define NORMAL_MEM_UNCACHED_COHERENT (SHARED|AP_RW|DOMAIN0|MEMWBWANC|DESC_SEC|XN)

/* device mapping type */
#define DEVICE_MEM     (SHARED|AP_RW|DOMAIN0|SHAREDEVICE|DESC_SEC|XN)

#define  CSP_GLOBAL_MODULE_NR       80
#define  CSP_GLOBAL_MOD_TYPE_NULL   0x00
#define  CSP_GLOBAL_MOD_TYPE_IO     0x01
#define  CSP_GLOBAL_MOD_TYPE_MEMORY 0x02

#define MEMS_TLB_L1_PTE_SECTION     0x00000002
#define MEMS_TLB_L1_PTE_COARSE      0x00000001
#define MEMS_TLB_L1_PTE_FINE        0x00000003
#define MEMS_TLB_L1_PTE_ERR         0x00000000
#define MEMS_TLB_L2_PTE_LARGE       0x00000001
#define MEMS_TLB_L2_PTE_SMALL       0x00000002
#define MEMS_TLB_L2_PTE_TINY        0x00000003
#define MEMS_TLB_L2_PTE_ERR         0x00000000

#define MEMS_TLB_L1_AP00            0x00000000
#define MEMS_TLB_L1_AP01            0x00000400
#define MEMS_TLB_L1_AP10            0x00000800
#define MEMS_TLB_L1_AP11            0x00000c00
#define MEMS_TLB_L2_AP00            0x00000000
#define MEMS_TLB_L2_AP01            0x00000010
#define MEMS_TLB_L2_AP10            0x00000020
#define MEMS_TLB_L2_AP11            0x00000030

#define MEMS_TLB_WBUF_ON            0x00000004
#define MEMS_TLB_CACHE_ON           0x00000008
#define MEMS_TLB_DOMAIN15           0x000001e0
#define MEMS_TLB_DOMAIN14           0x000001c0

/* COASE L2 AP */
#define MEMS_TLB_COASE_L2_AP00      0x00000000
#define MEMS_TLB_COASE_L2_AP01      0x00000550
#define MEMS_TLB_COASE_L2_AP10      0x00000aa0
#define MEMS_TLB_COASE_L2_AP11      0x00000ff0

#define MEMS_DOMAIN_ACCESS_VALU     0xc0000000    /* domain 15 is manager */
#define MEMS_ALIGN(val, align)      (((val) + ((align) - 1)) & ~((align) - 1))
#define MEMS_MIN(a, b)              ((a) < (b) ? (a) : (b))

#endif
