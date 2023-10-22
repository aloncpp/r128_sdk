#ifndef SUNXI_HAL_COMMON_H
#define SUNXI_HAL_COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif
#include <barrier.h>

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a,b)   ((a) < (b) ? (b) : (a))
#endif

#ifndef MIN
#define MIN(a, b)  ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)   ((a) < (b) ? (b) : (a))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)       (sizeof(x) / sizeof((x)[0]))
#endif

#define __ALIGN_KERNEL(x, a) __ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask) (((x) + (mask)) & ~(mask))

#ifndef ALIGN_UP
#define ALIGN_UP(x, a) __ALIGN_KERNEL((x), (a))
#endif

#ifndef ALIGN_DOWN
#define ALIGN_DOWN(x, a) __ALIGN_KERNEL((x) - ((a) - 1), (a))
#endif

#ifndef BIT
#define BIT(x) (1 << x)
#endif

#define get_bvalue(addr)	(*((volatile unsigned char  *)(addr)))
#define put_bvalue(addr, v)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define get_hvalue(addr)	(*((volatile unsigned short *)(addr)))
#define put_hvalue(addr, v)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define get_wvalue(addr)	(*((volatile unsigned int   *)(addr)))
#define put_wvalue(addr, v)	(*((volatile unsigned int   *)(addr)) = (unsigned int)(v))

#define set_byte(addr, v)    (*((volatile unsigned char  *)(addr)) |=  (unsigned char)(v))
#define clr_byte(addr, v)    (*((volatile unsigned char  *)(addr)) &= ~(unsigned char)(v))
#define set_half_word(addr, v)   (*((volatile unsigned short *)(addr)) |=  (unsigned short)(v))
#define clr_half_word(addr, v)   (*((volatile unsigned short *)(addr)) &= ~(unsigned short)(v))
#define set_word(addr, v)   (*((volatile unsigned int   *)(addr)) |=  (unsigned int)(v))
#define clr_word(addr, v)   (*((volatile unsigned int   *)(addr)) &= ~(unsigned int)(v))

/* common register access operation. */
#define hal_readb(reg)          ({uint8_t __v; dsb(); __v = *(volatile uint8_t  *)(long)(reg); __v;})
#define hal_readw(reg)          ({uint16_t __v; dsb(); __v = *(volatile uint16_t  *)(long)(reg); __v;})
#define hal_readl(reg)          ({uint32_t __v; dsb(); __v = *(volatile uint32_t  *)(long)(reg); __v;})
#define hal_writeb(value,reg)   ({isb(); *(volatile uint8_t  *)(long)(reg) = (value); dsb();})
#define hal_writew(value,reg)   ({isb(); *(volatile uint16_t  *)(long)(reg) = (value); dsb();})
#define hal_writel(value,reg)   ({isb(); *(volatile uint32_t  *)(long)(reg) = (value); dsb();})


#ifndef readb
#define readb(addr)	    hal_readb(addr)
#endif
#ifndef readw
#define readw(addr)	    hal_readw(addr)
#endif
#ifndef readl
#define readl(addr)	    hal_readl(addr)
#endif
#ifndef writeb
#define writeb(v, addr)	hal_writeb(v, addr)
#endif
#ifndef writew
#define writew(v, addr) hal_writew(v, addr)
#endif
#ifndef writel
#define writel(v, addr)	hal_writel(v, addr)
#endif

#define cmp_wvalue(addr, v) (v == (*((volatile unsigned int *) (addr))))

#ifndef OK
#define OK 	(0)
#endif
#ifndef FAIL
#define FAIL	(-1)
#endif
#ifndef TRUE
#define TRUE	(1)
#endif
#ifndef FALSE
#define	FALSE	(0)
#endif
#ifndef true
#define true     1
#endif
#ifndef false
#define false    0
#endif

#ifndef NULL
#define NULL    0
#endif

#define CACHELINE_LEN (64)

typedef uint64_t  u64;
typedef uint32_t  u32;
typedef uint16_t  u16;
typedef uint8_t   u8;
typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;

#define HAL_ARG_UNUSED(NAME)   (void)(NAME)

/* general function pointer defines */
typedef s32 (*__pCBK_t) (void *p_arg);			/* call-back */
typedef s32 (*__pISR_hdle_t) (void *p_arg);		/* ISR */

typedef s32(*__pNotifier_t) (u32 message, u32 aux);	/* notifer call-back */
typedef s32(*__pCPUExceptionHandler) (void);		/* cpu exception handler pointer */

#define BUG() do {                                                             \
        printf("BUG: failure at %s:%d/%s()!\n", __FILE__, __LINE__, __func__); \
        backtrace(NULL, NULL, 0, 0, printf);                                   \
        while(1);                                                              \
    } while (0)

#ifndef BUG_ON
#define BUG_ON(condition)   do { if (unlikely(condition)) BUG(); } while (0)
#define WARN_ON(condition)  ({                                          \
        int __ret_warn_on = !!(condition);                              \
        unlikely(__ret_warn_on);                                        \
    })
#endif

#ifndef WARN
#define WARN(condition, format...) ({                                   \
        int __ret_warn_on = !!(condition);                              \
        if(__ret_warn_on)                                               \
            printf(format);                                             \
        unlikely(__ret_warn_on);                                        \
    })
#endif

#ifdef CONFIG_DEBUG_BACKTRACE
#define hal_assert(ex)                                                  \
    if (!(ex)) {                                                        \
        printf("%s line %d, fatal error.\n", __func__, __LINE__);       \
        backtrace(NULL, NULL, 0, 0, printf);                            \
        while(1);                                                       \
    }
#else
#define hal_assert(ex)                                                  \
    if (!(ex)) {                                                        \
        printf("%s line %d, fatal error.\n", __func__, __LINE__);       \
        while(1);                                                       \
    }
#endif

// version combine.
#define SUNXI_HAL_VERSION_MAJOR_MINOR(major, minor)     (((major) << 8) | (minor))

typedef struct sunxi_hal_version
{
    // API version NO.
    uint16_t api;

    // Driver version NO.
    uint16_t drv;
} sunxi_hal_version_t;

// General return code of hal driver.
#define SUNXI_HAL_OK                     0UL
// Unspecified error.
#define SUNXI_HAL_ERROR                 -1UL
// Hal is busy.
#define SUNXI_HAL_ERROR_BUSY            -2UL
// Timout occured.
#define SUNXI_HAL_ERROR_TIMEOUT         -3UL
// Operaion not supported.
#define SUNXI_HAL_ERROR_UNSUPOT         -4UL
// Parameter error.
#define SUNXI_HAL_ERROR_PARAERR         -5UL
// Start of driver specific errors.
#define SUNXI_HAL_ERROR_DRVSPECIFIC     -6UL

typedef enum sunxi_hal_power_state
{
    ///< Power off: no operation possible
    SUSNXI_HAL_POWER_OFF,
    ///< Low Power mode: retain state, detect and signal wake-up events
    SUSNXI_HAL_POWER_LOW,
    ///< Power on: full operation at maximum performance
    SUSNXI_HAL_POWER_FULL
} sunxi_hal_power_state_e;

typedef int32_t (*poll_wakeup_func)(int32_t dev_id, short key);

typedef struct _sunxi_hal_poll_ops
{
    int32_t (* check_poll_state) (int32_t dev, short key);
    int32_t (* hal_poll_wakeup) (int32_t dev, short key);
    int32_t (* register_poll_wakeup) (poll_wakeup_func poll_wakeup);
} sunxi_hal_poll_ops;

/* bitops */
extern int fls(int x);

void hal_free_coherent(void *addr);
void *hal_malloc_coherent(size_t size);
void hal_free_coherent_align(void *addr);
void *hal_malloc_coherent_align(size_t size, int align);

#ifdef CONFIG_COMPONENTS_AMP
void *amp_align_malloc(int size);
void amp_align_free(void *ptr);
#endif

#ifdef __cplusplus
}
#endif

#endif  /*SUNXI_HAL_COMMON_H*/

