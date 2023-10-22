#ifndef __TYPES_H__
#define __TYPES_H__

#include <types.h>
#include <stdbool.h>

#include <sunxi_hal_common.h>

#ifndef min
#define min(a, b)  ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b)   ((a) < (b) ? (b) : (a))
#endif

/* return value defines */
#ifndef OK
#define OK    (0)
#endif
#ifndef FAIL
#define FAIL    (-1)
#endif
#ifndef TRUE
#define TRUE    (1)
#endif
#ifndef FALSE
#define FALSE    (0)
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

#if 0
/* general data type defines */
typedef	void * 		    HANDLE;
typedef unsigned long long  u64;
typedef unsigned int        u32;
typedef unsigned short      u16;
typedef unsigned char       u8;
typedef signed long long    s64;
typedef signed int          s32;
typedef signed short        s16;
typedef signed char         s8;
//typedef signed char         bool;
//typedef unsigned int        size_t;
typedef unsigned int        uint;
#endif

typedef u8  __u8;
typedef u16  __u16;
typedef u32  __u32;
typedef u64  __u64;
typedef s8  __s8;
typedef s16  __s16;
typedef s32  __s32;
typedef s64  __s64;

#endif /* __TYPES_H__ */
