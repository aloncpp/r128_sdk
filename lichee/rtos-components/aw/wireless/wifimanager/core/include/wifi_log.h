#ifndef __WIFI_LOG_H
#define __WIFI_LOG_H

#include <stdint.h>

#if __cplusplus
extern "C" {
#endif

typedef enum __wmg_log_level{
	WMG_MSG_NONE = 0,
	WMG_MSG_ERROR,
	WMG_MSG_WARNING,
	WMG_MSG_INFO,
	WMG_MSG_DEBUG,
	WMG_MSG_MSGDUMP,
	WMG_MSG_EXCESSIVE
}wmg_log_level_t;

typedef enum __wmg_log_setting{
	WMG_LOG_SETTING_NONE = 0,
	WMG_LOG_SETTING_TIME = 1 << 0,
	WMG_LOG_SETTING_FILE = 1 << 1,
	WMG_LOG_SETTING_FUNC = 1 << 2,
	WMG_LOG_SETTING_LINE = 1 << 3,
}wmg_log_setting_t;

void wmg_print(int level, int para_enable, const char *tag, const char *file, const char *func, int line, const char *fmt, ...);
void wmg_set_debug_level(wmg_log_level_t level);
int wmg_get_debug_level();
void wmg_set_log_para(int log_para);
int wmg_get_log_para();

#ifdef OS_NET_FREERTOS_OS

#define WMG_DEBUG(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 1, "WDG: ", NULL, NULL, 0, fmt,##arg)

#define WMG_DEBUG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_INFO(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 1, "WIF: ", NULL, NULL, 0, fmt,##arg)

#define WMG_INFO_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_WARNG(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 1, "WAR: ", NULL, NULL, 0, fmt,##arg)

#define WMG_WARNG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_ERROR(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 1, "WER: ", NULL, NULL, 0, fmt,##arg)

#define WMG_ERROR_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_DUMP(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 1, "WDP: ", NULL, NULL, 0, fmt,##arg)

#define WMG_DUMP_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_EXCESSIVE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 1, "WEX: ", NULL, NULL, 0, fmt,##arg)

#define WMG_EXCESSIVE_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 0, NULL, NULL, NULL, 0, fmt,##arg)

#else

#define WMG_DEBUG(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 1, "WDUG: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt, ##arg)

#define WMG_DEBUG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_DEBUG, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_INFO(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 1, "WINF: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_INFO_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_INFO, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_WARNG(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 1, "WWAR: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_WARNG_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_WARNING, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_ERROR(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 1, "WERR: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_ERROR_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_ERROR, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_DUMP(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 1, "WDUP: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_DUMP_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_MSGDUMP, 0, NULL, NULL, NULL, 0, fmt,##arg)

#define WMG_EXCESSIVE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 1, "WEXC: ", \
		(wmg_get_log_para() > 1)?__FILE__:NULL, \
		(wmg_get_log_para() > 1)?__func__:NULL, \
		(wmg_get_log_para() > 1)?__LINE__:0, \
		fmt,##arg)

#define WMG_EXCESSIVE_NONE(fmt,arg...) \
	wmg_print(WMG_MSG_EXCESSIVE, 0, NULL, NULL, NULL, 0, fmt,##arg)
#endif

#if __cplusplus
};  // extern "C"
#endif

#endif //__WIFI_LOG_H
