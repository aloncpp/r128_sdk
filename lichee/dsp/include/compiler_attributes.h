#ifndef __LINUX_COMPILER_ATTRIBUTES_H
#define __LINUX_COMPILER_ATTRIBUTES_H

#define __always_unused                 __attribute__((__unused__))
#define __maybe_unused                  __attribute__((__unused__))
#define barrier() __asm__ __volatile__("" : : : "memory")

#endif /* __LINUX_COMPILER_ATTRIBUTES_H */
