#ifndef __AACTD_LOCAL_PROCESS_H__
#define __AACTD_LOCAL_PROCESS_H__

#include "aactd/communicate.h"

#define AACTD_VERSION "AACTD-V0.0.1"

#define LISTEN_BACKLOG 5

#define COM_BUF_LEN_MAX_DEFAULT 1024

/* All com_buf in aactd use this length to allocate memory. */
extern unsigned int com_buf_len_max;

extern int verbose_level;

#define AACTD_DEBUG(level, format, arg...) \
    do { \
        if (verbose_level >= level) { \
            printf("[DEBUG](%s:%d): "format, __func__, __LINE__, ##arg); \
        } \
    } while (0)

int local_process_init(void);
int local_process_release(void);

int read_com_from_local(struct aactd_com *com);
int write_com_to_local(const struct aactd_com *com);

#endif /* ifndef __AACTD_LOCAL_PROCESS_H__ */
