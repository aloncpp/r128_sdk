/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
* Change Logs:
* Date           Author       Notes
* 2015-03-07     Bernard      Add copyright header.
*/
#include "crt.h"
#include <stdio.h>

void *operator new(size_t size)
{
    return malloc(size);
}

void *operator new[](size_t size)
{
    return malloc(size);
}

void operator delete(void *ptr)
{
    free(ptr);
}

void operator delete[](void *ptr)
{
    return free(ptr);
}

void __cxa_pure_virtual(void)
{
    printf("Illegal to call a pure virtual function.\n");
}
