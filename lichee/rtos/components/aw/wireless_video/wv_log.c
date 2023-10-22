#include "wv_log.h"

wv_log_level_t log_print_level = L_DEBUG;

int wv_log_set_level(wv_log_level_t level)
{
    if (level >= L_NONE) {
        log_print_level = level;
        return 0;
    } else {
        printf("Illegal log level value!\n");
        return -1;
    }
}

wv_log_level_t wv_log_get_level(void)
{
    return log_print_level;
}
