#ifndef _VIRT_LOG_H_
#define _VIRT_LOG_H_

void virt_log_put(char ch);
int virt_log_put_buf(char *buf);
int virt_log_put_buf_len(char *buf, int len);
int virt_log_flush(void);
int virt_log_is_enable(void);
void virt_log_enable(int enable);


#endif

