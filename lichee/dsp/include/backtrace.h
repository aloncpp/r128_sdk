#ifndef BACKTRACE_H
#define BACKTRACE_H

typedef int (*print_function)(const char *fmt, ...);

extern int backtrace_exception(print_function print_func,
                        void *frame);

extern int backtrace(char *taskname, void *trace[], int size, int offset, print_function print_func);

#ifdef CONFIG_DEBUG_BACKTRACE
extern int arch_backtrace_exception(print_function print_func,
                        void *frame);
extern int arch_backtrace(char *taskname, void *trace[], int size, int offset, print_function print_func);
extern void show_exception_information_for_core(const void *f, int core);
#endif

#endif  /*BACKTRACE_H*/
