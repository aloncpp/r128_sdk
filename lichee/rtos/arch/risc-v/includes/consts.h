#ifndef _RTOS_CONST_CONST_H
#define _RTOS_CONST_CONST_H

#ifdef __ASSEMBLY__
#define _AC(X,Y)    X
#define _AT(T,X)    X
#else
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define __AC(X,Y)   (X##Y)
#define _AC(X,Y)    __AC(X,Y)
#define _AT(T,X)    ((T)(X))

typedef unsigned long phys_addr_t;
#endif

#define _UL(x)      (_AC(x, UL))
#define _ULL(x)     (_AC(x, ULL))

#define _BITUL(x)   (_UL(1) << (x))
#define _BITULL(x)  (_ULL(1) << (x))

#endif  //_RTOS_CONST_CONST_H
