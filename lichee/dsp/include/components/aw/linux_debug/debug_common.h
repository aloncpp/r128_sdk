#ifndef __DEBUG_COMMON_H__
#define __DEBUG_COMMON_H__

extern void debug_common_init(void);
extern int log_save(const char *fmt, ...);
extern void log_mutex_init(void);
extern void SaveLogMutexDisable(void);

#define pr_info_thread(fmt, arg...)		log_save("[dsp:info]-"fmt, ##arg)
#define pr_err_thread(fmt, arg...)		log_save("[dsp:err]- fun:%s() line:%d - "fmt, __func__, __LINE__, ##arg)

struct debug_img_t {
	uint32_t log_reset;
	uint32_t log_head_addr;
	uint32_t log_end_addr;
	uint32_t log_head_size;
};

#endif



