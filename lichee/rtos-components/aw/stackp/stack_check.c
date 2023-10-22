#include <stdlib.h>
#include <stdio.h>

#ifdef CONFIG_DEBUG_BACKTRACE
#include <backtrace.h>
#endif

#define  STACK_CHECK_MAGICNUMBER    (0x5a5a5a5a)

void __stack_chk_fail(void)
{
    printf("\nfata error: stack corrupted!\n");

#ifdef CONFIG_DEBUG_BACKTRACE
    backtrace(NULL, NULL, 0, 0, printf);
#endif
    while(1);
}

void __stack_chk_fail_local (void)
{
    __stack_chk_fail();
}

unsigned long __stack_chk_guard = STACK_CHECK_MAGICNUMBER;

void stack_protector_init(void)
{
    __stack_chk_guard = STACK_CHECK_MAGICNUMBER;
}

