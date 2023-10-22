#include "btmg_log.h"

bt_log_level_t log_level = MSG_INFO;
static int btmg_debug_mask = 0;

int bt_set_debug_level(bt_log_level_t level)
{
    if (level >= MSG_NONE) {
        log_level = level;
        return 0;
    } else {
        printf("Illegal log level value!\n");
        return -1;
    }
}

bt_log_level_t bt_get_debug_level(void)
{
    return log_level;
}

int  bt_get_debug_mask(void)
{
    return btmg_debug_mask;
}

bt_log_level_t bt_set_debug_mask(int mask)
{
    btmg_debug_mask = mask;
}

int  bt_check_debug_mask(int mask)
{
    return btmg_debug_mask & mask;
}
