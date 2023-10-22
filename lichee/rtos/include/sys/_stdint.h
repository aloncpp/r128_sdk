/*
 * Copyright (c) 2004, 2005 by
 * Ralf Corsepius, Ulm/Germany. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * is freely granted, provided that this notice is preserved.
 */

#ifndef _SYS__STDINT_H
#define _SYS__STDINT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char int8_t ;
#define _INT8_T_DECLARED

typedef unsigned char uint8_t ;
#define _UINT8_T_DECLARED

#define __int8_t_defined 1

typedef short int16_t ;
#define _INT16_T_DECLARED

typedef unsigned short uint16_t ;
#define _UINT16_T_DECLARED
#define __int16_t_defined 1

typedef int int32_t ;
#define _INT32_T_DECLARED

typedef unsigned int uint32_t ;
#define _UINT32_T_DECLARED
#define __int32_t_defined 1

typedef long long int64_t ;
#define _INT64_T_DECLARED

typedef unsigned long long uint64_t ;
#define _UINT64_T_DECLARED

#define __int64_t_defined 1

typedef signed long long intmax_t;
#define _INTMAX_T_DECLARED

typedef unsigned long long uintmax_t;
#define _UINTMAX_T_DECLARED

typedef long int intptr_t;
#define _INTPTR_T_DECLARED

typedef unsigned long int uintptr_t;
#define _UINTPTR_T_DECLARED

#ifdef __cplusplus
}
#endif

#endif /* _SYS__STDINT_H */
