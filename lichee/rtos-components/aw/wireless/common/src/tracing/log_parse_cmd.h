#ifndef __LOG_PARSE_CMD_H__
#define __LOG_PARSE_CMD_H__

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int log_parse_cmd_start(char *fifo_path);
int log_parse_cmd_stop(void);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif //__LOG_PARSE_CMD_H__