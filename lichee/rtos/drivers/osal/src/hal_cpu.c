#include <compiler.h>

__weak unsigned long arch_cpu_id(void)
{
    return 0L;
}

unsigned long hal_cpu_get_id(void)
{
#ifdef CONFIG_ARCH_SUN20IW2
#ifdef CONFIG_ARCH_ARM
    return 0L;
#elif defined(CONFIG_ARCH_RISCV)
    return 1L;
#endif
#elif defined(CONFIG_ARCH_SUN8IW18)
    int cur_cpu_id(void);
    return cur_cpu_id();
#endif
}
