#ifndef _APP_COMMON_H_
#define _APP_COMMON_H_


//pointer converter
#define PTR_TO_UINT(p) ((unsigned int) ((uintptr_t) (p)))
#define UINT_TO_PTR(u) ((void *) ((uintptr_t) (u)))

#define PTR_TO_INT(p) ((int) ((intptr_t) (p)))
#define INT_TO_PTR(u) ((void *) ((intptr_t) (u)))


//log
#ifdef DEBUG
#define pr_debug(fmt, ...) \
    printf("%s:%s() "fmt"\n", __FILE__, __func__, ##__VA_ARGS__)
#else
#define pr_debug(fmt, ...) do { ; } while (0)
#endif
#define pr_info(fmt, ...) \
    printf("%s:%s() "fmt"\n", __FILE__, __func__, ##__VA_ARGS__)
#define pr_warn(fmt, ...) \
    printf("%s:%s() "fmt"\n", __FILE__, __func__, ##__VA_ARGS__)
#define pr_err(fmt, ...) \
    printf("%s:%s() "fmt"\n", __FILE__, __func__, ##__VA_ARGS__)



#endif
