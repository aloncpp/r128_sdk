#ifndef __LOG_WORKER_H__
#define __LOG_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif

int log_worker_init();
int log_worker_deinit();
void *log_worker_loop(void *arg);
int log_worker_schedule(void *(*function)(void *), void *arg, void **ret);

#ifdef __cplusplus
}
#endif

#endif /* __LOG_WORKER_H__ */